//=============================================================================
// Skeleton from: https://github.com/banach-space/llvm-tutor/
//
// FILE:
//    GlobalVarAccess.cpp
//=============================================================================

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ErrorHandling.h>
#include <string>
#include <vector>

using namespace llvm;

namespace llvm {
typedef DenseMap<std::pair<GlobalVariable *, std::vector<unsigned int>>,
                 std::pair<GlobalVariable *, GlobalVariable *>>
    CounterMap;

// Implement vector info, otherwise LLVM complains
template <> struct DenseMapInfo<std::vector<unsigned int>> {
  static std::vector<unsigned int> getEmptyKey() {
    // Return an empty vector as the empty key
    return {};
  }

  static std::vector<unsigned int> getTombstoneKey() {
    // Tombstone key (can be a vector with an invalid value)
    return {std::numeric_limits<unsigned int>::max()};
  }

  static unsigned getHashValue(const std::vector<unsigned int> &vec) {
    unsigned hash = 0;
    for (unsigned int val : vec) {
      hash = hash * 31 + val;
    }
    return hash;
  }

  static bool isEqual(const std::vector<unsigned int> &a, const std::vector<unsigned int> &b) {
    return a == b; // Compare the vectors for equality
  }
};

} // namespace llvm

namespace {

struct GlobalVarCounters : public PassInfoMixin<GlobalVarCounters> {

  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {

    errs() << "OK BOOTING UP\n";

    // Get global data context
    LLVMContext &Context = M.getContext();

    // Map each global variable field to its corresponding counter for <read,
    // write>
    DenseMap<GlobalVariable *, std::pair<GlobalVariable *, GlobalVariable *>> globalCounters;
    CounterMap globalCtrs;

    // Create a new pair of constants for all globals
    for (GlobalVariable &GV : M.globals()) {
      if (GV.isConstant() || GV.getName() == "llvm.global_ctors" ||
          GV.getName().contains("_RD_CNT") || GV.getName().contains("_WR_CNT"))
        continue;

      // Recurse all the fields in the struct (more precision)
      if (GV.getValueType()->isStructTy()) {
        instrumentStructFields(M, Context, &GV, cast<StructType>(GV.getValueType()),
                               demangle(GV.getName().str()));
      }
      // A regular primitive (which we handle right away)
      else {
        // Create a unique counter variable for each global variable
        auto *rd_counter = new GlobalVariable(
            M, Type::getInt32Ty(Context), false, GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt32Ty(Context), 0), GV.getName() + "_RD_CNT");
        auto *wr_counter = new GlobalVariable(
            M, Type::getInt32Ty(Context), false, GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt32Ty(Context), 0), GV.getName() + "_WR_CNT");

        // Counter for primitive
        errs() << "Instrumented Counter for: " << demangle(GV.getName()) << "\n";

        // Store the mapping (global primitive vars have just an empty set, since there is no path)
        globalCtrs[std::pair(&GV, std::vector<unsigned int>())] = std::pair(rd_counter, wr_counter);
      }
    }

    errs() << "Created GLOBAL CNTRS\n";

    // Instrument every function that reads/writes a global
    for (Function &F : M) {

      // Skip declarations - only definitions
      if (F.isDeclaration())
        continue;

      for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
          // Handle r/w in every fn instruction of module
          handleMemoryAccess(M, F, &I, globalCtrs);
        }
      }
    }

    // // Write all the accesses to stdout (for now)
    createDumpFunction(M, globalCounters);

    return PreservedAnalyses::all();
  }

  std::vector<unsigned int> getFieldPathFromGEP(GetElementPtrInst *GEP) {
    // Access a getelementpointer:
    // - we have a load and instruct the operand
    // - afterward, we can:
    // (a) either find the correct element, we access
    // (b) abort, if the element is not correctly found
    // [This is not applicable to "normal" load/store instructions, which dont
    // have structs/arrs]
    //
    // Example (we access lock_guard at 0 at 0)
    // %6 = getelementptr inbounds %"class.std::lock_guard", ptr %5, i32 0, i32 0
    // %7 = load ptr, ptr %4, align 8

    std::vector<unsigned int> fieldPath;
    Type *currentType = GEP->getPointerOperandType()->getPointerTo();

    // Iterate over GEP indices
    for (auto it = GEP->idx_begin() + 1; it != GEP->idx_end(); ++it) {

      // Can only do instrumentation, if the index is known (simplified)
      if (ConstantInt *CI = dyn_cast<ConstantInt>(it->get())) {
        // Extract the GEP index for the fieldPath
        unsigned index = CI->getZExtValue();
        fieldPath.push_back(index);

        // Update currentType for next level
        if (currentType->isStructTy()) {
          StructType *structType = cast<StructType>(currentType);

          // The index must be in bounds, otherwise, we should abort
          if (index < structType->getNumElements()) {
            // The next type is taken, which is evaluated, against the index
            currentType = structType->getElementType(index);
          } else {
            std::string err_msg = "We access index " + std::to_string(index) +
                                  " on a struct that has " +
                                  std::to_string(structType->getNumElements()) + " fields!";
            report_fatal_error(err_msg.c_str());
            break;
          }
        } else if (currentType->isArrayTy()) {
          currentType = currentType->getArrayElementType();
        } else {
          // Not a composite
          break;
        }
      }
    }

    return fieldPath;
  }

  void handleMemoryAccess(Module &M, Function &F, Instruction *I, CounterMap &structFieldCounters) {
    Value *pointerOperand = nullptr;

    if (auto *loadInst = dyn_cast<LoadInst>(I)) {
      pointerOperand = loadInst->getPointerOperand();
    } else if (auto *storeInst = dyn_cast<StoreInst>(I)) {
      pointerOperand = storeInst->getPointerOperand();
    }

    // errs() << "(" << I->getOpcodeName() << ") in: " << demangle(F.getName().str()) << "\n";

    if (!pointerOperand)
      return;

    // Check if pointer is derived from a GEP (it is a struct!)
    if (auto *gep = dyn_cast<GetElementPtrInst>(pointerOperand)) {

      errs() << "Got a GEP\n";

      // Get the base variable (e.g., global variable)
      Value *base = gep->getPointerOperand();
      // Get the global var (recursively)
      GlobalVariable *GV = getGlobalVariable(base);

      // Could be null, if not a GV
      if (GV) {

        errs() << "accessed GV_ptr: " << GV->getName() << "\n";
        // Extract the field path for the GEP access
        std::vector<unsigned> fieldPath = getFieldPathFromGEP(gep);
        errs() << "Accessed field path of '" << demangle(GV->getName()) << "'in '"
               << demangle(F.getName()) << "'";
        for (auto idx : fieldPath)
          errs() << idx << " ";
        errs() << "\n";

        // Get the counters for the var and the referenced field
        auto it = structFieldCounters.find({GV, fieldPath});
        if (it != structFieldCounters.end()) {
          auto counters = it->second;

          // Increment the appropriate counter
          IRBuilder<> builder(I);
          GlobalVariable *counter = (isa<LoadInst>(I)) ? counters.first : counters.second;
          Value *counterVal = builder.CreateLoad(Type::getInt32Ty(M.getContext()), counter);
          Value *incremented =
              builder.CreateAdd(counterVal, ConstantInt::get(Type::getInt32Ty(M.getContext()), 1));
          builder.CreateStore(incremented, counter);

          errs() << "Accessed field path of '" << demangle(GV->getName()) << "'in '"
                 << demangle(F.getName()) << "'";
          for (auto idx : fieldPath)
            errs() << idx << " ";
          errs() << "\n";
        }
      } else {
        // TODO: Try to find out the type of the L
        Value *pointerOperand = nullptr;

        if (auto *loadInst = dyn_cast<LoadInst>(I)) {
          pointerOperand = loadInst->getPointerOperand();
          // errs() << "base is load...\n";
        } else if (auto *storeInst = dyn_cast<StoreInst>(I)) {
          pointerOperand = storeInst->getPointerOperand();
          // errs() << "base is store...\n";
        }

        if (!pointerOperand) {
          errs() << "base is (unexpected?):" << *base << "\n";
          return;
        }

        if (dyn_cast<GlobalVariable>(pointerOperand)) {
          errs() << "NOW WE TALK\n";
        } else {
        }
      }
    }
    // If it is not, it might be a variable, that is a primitive type
    else if (dyn_cast<GlobalVariable>(pointerOperand)) {

      errs() << "accessing direct GV\n";
      // Get the element from the map and increment
      auto it = structFieldCounters.find(
          {dyn_cast<GlobalVariable>(pointerOperand), std::vector<unsigned int>()});

      if (it != structFieldCounters.end()) {
        auto counters = it->second;

        // Increment the appropriate counter
        IRBuilder<> builder(I);
        GlobalVariable *counter = (isa<LoadInst>(I)) ? counters.first : counters.second;
        Value *counterVal = builder.CreateLoad(Type::getInt32Ty(M.getContext()), counter);
        Value *incremented =
            builder.CreateAdd(counterVal, ConstantInt::get(Type::getInt32Ty(M.getContext()), 1));
        builder.CreateStore(incremented, counter);
        errs() << *I << "\nINSTRUMENTED\n";
      }
    }
    // Otherwise, we don't care
  }

  void instrumentStructFields(Module &M, LLVMContext &Context, GlobalVariable *GV,
                              StructType *structType, std::string fieldPrefix) {
    for (unsigned i = 0; i < structType->getNumElements(); ++i) {
      // Get the type of all elements inside the struct
      Type *fieldType = structType->getElementType(i);

      // "" + GV_NAME + "." + "el{FIELD_NUM}" -- debug field names is a bit
      // hard ^^; For later: Look up DICompositeType, DIDerivedType
      // TODO: Add names for easier understanding
      std::string fieldName = fieldPrefix + ".el" + std::to_string(i);

      // Check the nested fields; could be anything from:
      // https://llvm.org/docs/LangRef.html#aggregate-types
      // But for now we just care about the structs
      // A struct is handled Recursively (array and others, we ignore)
      if (fieldType->isStructTy()) {
        instrumentStructFields(M, Context, GV, cast<StructType>(fieldType), fieldName);
      } else {
        createCounterVariables(M, Context, fieldName);
      }
    }
  }

  // Go recursively until the GV that is written, is found
  GlobalVariable *getGlobalVariable(Value *V) {
    if (auto *GV = dyn_cast<GlobalVariable>(V)) {
      // Base case: This is the global variable we are looking for
      return GV;
    } else if (auto *allocaInst = dyn_cast<AllocaInst>(V)) {
      // Alloca: We are in a function call, where arguments are passed
      // This means, we cannot infer at compile time, what is done
      return nullptr;
    } else if (auto *loadInst = dyn_cast<LoadInst>(V)) {
      // Load instruction: Check the operand it's loading from
      return getGlobalVariable(loadInst->getPointerOperand());
    } else if (auto *storeInst = dyn_cast<StoreInst>(V)) {
      // Store instruction: Check the value being stored
      return getGlobalVariable(storeInst->getValueOperand());
    } else if (auto *gep = dyn_cast<GetElementPtrInst>(V)) {
      // GEP instruction: Check its pointer operand
      return getGlobalVariable(gep->getPointerOperand());
    } else if (auto *bitcast = dyn_cast<BitCastInst>(V)) {
      // Bitcast instruction: Follow the operand
      return getGlobalVariable(bitcast->getOperand(0));
    } else if (auto *phi = dyn_cast<PHINode>(V)) {
      // PHI node: Check all incoming values
      for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
        if (auto *GV = getGlobalVariable(phi->getIncomingValue(i)))
          return GV;
      }
    } else if (auto *arg = dyn_cast<Argument>(V)) {
      // Function argument: This might point to a global variable passed as a parameter
      errs() << "Argument traced, we don't go on: " << *arg << "\n";
      return nullptr;
    }

    errs() << *V;
    errs() << " ==  No match!\n";
    // No global variable found
    return nullptr;
  }

  void analyzeGEPInstruction(GetElementPtrInst *GEP, Module &M) {
    Type *baseType = GEP->getPointerOperandType();
    StructType *structType = dyn_cast<StructType>(baseType->getPointerTo());

    if (structType && GEP->getNumIndices() > 1) {
      auto it = GEP->idx_begin();
      ++it; // Skip the first index (struct base offset)

      std::string fieldAccessPath = structType->getName().str();
      for (; it != GEP->idx_end(); ++it) {
        if (ConstantInt *CI = dyn_cast<ConstantInt>(it)) {
          unsigned fieldIndex = CI->getZExtValue();
          fieldAccessPath += ".el" + std::to_string(fieldIndex);
          if (fieldIndex < structType->getNumElements() &&
              structType->getElementType(fieldIndex)->isStructTy()) {
            structType = dyn_cast<StructType>(structType->getElementType(fieldIndex));
          }
        }
      }

      errs() << "Field accessed: " << fieldAccessPath << "\n";
    }
  }

  void createCounterVariables(Module &M, LLVMContext &Context, const std::string &name) {
    auto *rd_counter =
        new GlobalVariable(M, Type::getInt32Ty(Context), false, GlobalValue::ExternalLinkage,
                           ConstantInt::get(Type::getInt32Ty(Context), 0), name + "_RD_CNT");
    auto *wr_counter =
        new GlobalVariable(M, Type::getInt32Ty(Context), false, GlobalValue::ExternalLinkage,
                           ConstantInt::get(Type::getInt32Ty(Context), 0), name + "_WR_CNT");

    errs() << "Instrumented Counter for: " << demangle(name) << "\n";
  }

  void incrementCounter(
      GlobalVariable *globalVar,
      DenseMap<GlobalVariable *, std::pair<GlobalVariable *, GlobalVariable *>> &globalCounters,
      Instruction *insertPoint) {
    IRBuilder<> builder(insertPoint);
    GlobalVariable *counter;

    // Load the current value of the counter and increment it
    if (dyn_cast<LoadInst>(insertPoint)) {
      counter = globalCounters[globalVar].first;
    } else {
      counter = globalCounters[globalVar].second;
    }

    Value *intCounter =
        builder.CreateBitCast(counter, Type::getInt32Ty(builder.getContext())->getPointerTo());
    Value *counterVal = builder.CreateLoad(Type::getInt32Ty(builder.getContext()), intCounter);
    Value *incremented =
        builder.CreateAdd(counterVal, ConstantInt::get(Type::getInt32Ty(builder.getContext()), 1));
    builder.CreateStore(incremented, counter);
  }

  // Function to create the DUMP function in the module
  void createDumpFunction(
      Module &M,
      DenseMap<GlobalVariable *, std::pair<GlobalVariable *, GlobalVariable *>> &globalCounters) {
    LLVMContext &Context = M.getContext();
    IRBuilder<> builder(Context);

    // Define `void DUMP_GLOB()`
    Function *dumpFunc = M.getFunction("DUMP_GLOB");

    // Get the defintion and replace it (no conflict resolution)
    if (!dumpFunc) {
      FunctionType *funcType = FunctionType::get(Type::getVoidTy(Context), false);
      dumpFunc = Function::Create(funcType, Function::ExternalLinkage, "DUMP_GLOB", M);
    }

    // Create the entry block
    BasicBlock *entry = BasicBlock::Create(Context, "entry", dumpFunc);
    builder.SetInsertPoint(entry);

    // Declare printf function
    FunctionCallee printfFunc = M.getOrInsertFunction(
        "printf", FunctionType::get(IntegerType::getInt32Ty(Context),
                                    {PointerType::get(Type::getInt8Ty(Context), 0)}, true));

    // Format string for printing each variable and counter
    Value *format_str = builder.CreateGlobalStringPtr("%s=[%d:%d]\n");

    for (auto &entry : globalCounters) {
      GlobalVariable *globalVar = entry.first;
      GlobalVariable *rd_counter = entry.second.first;
      GlobalVariable *wr_counter = entry.second.second;

      // Load the counter values and print them
      Value *rd_counterVal = builder.CreateLoad(rd_counter->getType(), rd_counter);
      Value *wr_counterVal = builder.CreateLoad(wr_counter->getType(), wr_counter);

      builder.CreateCall(printfFunc,
                         {format_str,
                          builder.CreateGlobalStringPtr(demangle(globalVar->getName().str())),
                          rd_counterVal, wr_counterVal});

      // Reset the counter (store ptr) for next time
      Value *zero = ConstantInt::get(Type::getInt32Ty(builder.getContext()), 0);
      builder.CreateStore(zero, rd_counter);
      builder.CreateStore(zero, wr_counter);
    }

    // Return from DUMP
    builder.CreateRetVoid();
  }

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getGlobalVarAccessPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "GlobalVarCounters", LLVM_VERSION_STRING, [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "global_var_counters") {
                    FPM.addPass(GlobalVarCounters());
                    return true;
                  }
                  return false;
                });
          }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize HelloWorld when added to the pass pipeline on the
// command line, i.e. via '-passes=hello-world'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return getGlobalVarAccessPluginInfo();
}
