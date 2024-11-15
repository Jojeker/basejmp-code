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
#include <llvm/Support/Casting.h>

using namespace llvm;

namespace
{


struct GlobalVarCounters : public PassInfoMixin<GlobalVarCounters>
{

  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  PreservedAnalyses
  run (Module &M, ModuleAnalysisManager &)
  {

    errs () << "OK BOOTING UP\n";

    // Get global data context
    LLVMContext &Context = M.getContext ();

    // Map each global variable to its corresponding counter for <read, write>
    DenseMap<GlobalVariable *, std::pair<GlobalVariable *, GlobalVariable*>> globalCounters;

    // Create a new pair of constants for all globals
    for (GlobalVariable &GV : M.globals ())
      {
        if (GV.isConstant () || GV.getName () == "llvm.global_ctors"
            || GV.getName ().contains ("_RD_CNT")
            || GV.getName ().contains ("_WR_CNT"))
          continue;

        // Create a unique counter variable for each global variable
        auto *rd_counter = new GlobalVariable (
            M, Type::getInt32Ty (Context), false, GlobalValue::ExternalLinkage,
            ConstantInt::get (Type::getInt32Ty (Context), 0),
            GV.getName () + "_RD_CNT");
        auto *wr_counter = new GlobalVariable (
            M, Type::getInt32Ty (Context), false, GlobalValue::ExternalLinkage,
            ConstantInt::get (Type::getInt32Ty (Context), 0),
            GV.getName () + "_WR_CNT");

        errs () << "Counter " << GV.getName () << " instrumented\n";
        // Store the mapping
        globalCounters[&GV] = std::pair(rd_counter,wr_counter);
      }

    errs () << "Created GLOBAL CNTRS\n";

    // Instrument every function that reads/writes a global
    for (Function &F : M)
      {

        // Skip declarations - only definitions
        if (F.isDeclaration ())
          continue;

        for (BasicBlock &BB : F)
          {
            for (Instruction &I : BB)
              {

                // Check the access type and increment accordingly
                if (LoadInst *loadInst = dyn_cast<LoadInst> (&I))
                  {
                    if (GlobalVariable *gv = dyn_cast<GlobalVariable> (
                            loadInst->getPointerOperand ()))
                      {
                        errs () << "(" << I.getOpcodeName ()
                                << ") in: " << demangle (F.getName ().str ())
                                << "\n";
                        incrementCounter (gv, globalCounters, loadInst);
                      }
                  }

                if (StoreInst *storeInst = dyn_cast<StoreInst> (&I))
                  {
                    if (GlobalVariable *gv = dyn_cast<GlobalVariable> (
                            storeInst->getPointerOperand ()))
                      {
                        errs () << "(" << I.getOpcodeName ()
                                << ") in:  " << demangle (F.getName ().str ())
                                << "\n";
                        incrementCounter (gv, globalCounters, storeInst);
                      }
                  }
              }
          }
      }

    // // Write all the accesses to stdout (for now)
    createDumpFunction (M, globalCounters);

    return PreservedAnalyses::all ();
  }

  void
  incrementCounter (
      GlobalVariable                               *globalVar,
      DenseMap<GlobalVariable *, std::pair<GlobalVariable *, GlobalVariable*>> &globalCounters,
      Instruction                                  *insertPoint)
  {
    IRBuilder<>     builder (insertPoint);
    GlobalVariable *counter;

    // Load the current value of the counter and increment it
    if(dyn_cast<LoadInst>(insertPoint)){
      counter = globalCounters[globalVar].first;
    } else {
      counter = globalCounters[globalVar].second;
    }

    Value *intCounter       = builder.CreateBitCast (
        counter, Type::getInt32Ty (builder.getContext ())->getPointerTo ());
    Value *counterVal = builder.CreateLoad (
        Type::getInt32Ty (builder.getContext ()), intCounter);
    Value *incremented = builder.CreateAdd (
        counterVal,
        ConstantInt::get (Type::getInt32Ty (builder.getContext ()), 1));
    builder.CreateStore (incremented, counter);
  }

  // Function to create the DUMP function in the module
  void
  createDumpFunction (
      Module                                       &M,
      DenseMap<GlobalVariable *, std::pair<GlobalVariable *, GlobalVariable*>> &globalCounters)
  {
    LLVMContext &Context = M.getContext ();
    IRBuilder<>  builder (Context);

    // Define `void DUMP_GLOB()` 
    Function *dumpFunc = M.getFunction("DUMP_GLOB");

    // Get the defintion and replace it (no conflict resolution)
    if (!dumpFunc) {
        FunctionType *funcType = FunctionType::get(Type::getVoidTy(Context), false);
        dumpFunc = Function::Create(funcType, Function::ExternalLinkage, "DUMP_GLOB", M);
    }

    // Create the entry block
    BasicBlock *entry  = BasicBlock::Create (Context, "entry", dumpFunc);
    builder.SetInsertPoint (entry);

    // Declare printf function
    FunctionCallee printfFunc = M.getOrInsertFunction (
        "printf",
        FunctionType::get (IntegerType::getInt32Ty (Context),
                           { PointerType::get (Type::getInt8Ty (Context), 0) },
                           true));

    // Format string for printing each variable and counter
    Value *format_str = builder.CreateGlobalStringPtr ("%s=[%d:%d]\n");

    for (auto &entry : globalCounters)
      {
        GlobalVariable *globalVar = entry.first;
        GlobalVariable *rd_counter   = entry.second.first;
        GlobalVariable *wr_counter   = entry.second.second;

        // Load the counter values and print them
        Value *rd_counterVal = builder.CreateLoad (rd_counter->getType (), rd_counter);
        Value *wr_counterVal = builder.CreateLoad (wr_counter->getType (), wr_counter);

        builder.CreateCall (printfFunc, { format_str, builder.CreateGlobalStringPtr (demangle(globalVar->getName ().str())),
                                          rd_counterVal, wr_counterVal });

        // Reset the counter (store ptr) for next time
        Value * zero = ConstantInt::get(Type::getInt32Ty(builder.getContext()), 0);
        builder.CreateStore(zero, rd_counter);
        builder.CreateStore(zero, wr_counter);
      }

    // Return from DUMP
    builder.CreateRetVoid ();
  }

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool
  isRequired ()
  {
    return true;
  }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo
getGlobalVarAccessPluginInfo ()
{
  return { LLVM_PLUGIN_API_VERSION, "GlobalVarCounters", LLVM_VERSION_STRING,
           [] (PassBuilder &PB) {
             PB.registerPipelineParsingCallback (
                 [] (StringRef Name, ModulePassManager &FPM,
                     ArrayRef<PassBuilder::PipelineElement>) {
                   if (Name == "global_var_counters")
                     {
                       FPM.addPass (GlobalVarCounters ());
                       return true;
                     }
                   return false;
                 });
           } };
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize HelloWorld when added to the pass pipeline on the
// command line, i.e. via '-passes=hello-world'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo ()
{
  return getGlobalVarAccessPluginInfo ();
}
