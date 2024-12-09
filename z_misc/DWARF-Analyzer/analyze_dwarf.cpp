
#include <llvm/ADT/StringRef.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/DebugInfo/DWARF/DWARFDataExtractor.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/DebugInfo/DWARF/DWARFExpression.h>
#include <llvm/DebugInfo/DWARF/DWARFFormValue.h>
#include <llvm/DebugInfo/DWARF/DWARFLocationExpression.h>
#include <llvm/DebugInfo/DWARF/DWARFSection.h>
#include <llvm/DebugInfo/DWARF/DWARFUnit.h>
#include <llvm/Object/ELF.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/DataExtractor.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdint>
#include <iostream>
#include <optional>
#include <stack>
#include <string>
#include <vector>

using namespace llvm;
using namespace llvm::dwarf;

// Helper for section data extraction
struct SectionInfo {
  std::string name;
  uint64_t start_address;
  uint64_t end_address;
};

struct StructMemberInfo {
  std::string name;
  uint64_t offset;
  uint64_t size;
};

std::optional<uint64_t> findSizeOfVariable(const llvm::DWARFDie &variable_die) {
  // Need to get size of the type of the variable
  std::optional<DWARFFormValue> typeResult = variable_die.find(DW_AT_type);
  if (!typeResult) {
    llvm::errs() << "Variable has no type information.\n";
    return std::nullopt;
  }

  llvm::DWARFDie type_die =
      variable_die.getDwarfUnit()->getDIEForOffset(*typeResult->getAsReference());
  if (!type_die) {
    llvm::errs() << "Failed to resolve type DIE of variable!\n";
    return std::nullopt;
  }

  // The type could be a pointer, so we need to resolve it
  while (type_die.getTag() == DW_TAG_pointer_type) {
    std::optional<DWARFFormValue> newTypeResult = type_die.find(DW_AT_type);
    if (!newTypeResult) {
      llvm::errs() << "Pointer type has no type information.\n";
      return std::nullopt;
    }
    type_die = type_die.getDwarfUnit()->getDIEForOffset(*newTypeResult->getAsReference());
    if (!type_die) {
      llvm::errs() << "Failed to resolve type DIE of pointer type!\n";
      return std::nullopt;
    }
  }

  // If it is an array, get the size of the array
  if (type_die.getTag() == DW_TAG_array_type) {
    std::optional<DWARFFormValue> countResult = type_die.find(DW_AT_count);
    if (!countResult) {
      llvm::errs() << "Failed to get count of array.\n";
      return std::nullopt;
    }
    // Multiply the size of the type with the count
    return countResult->getAsUnsignedConstant().value();
  }

  std::optional<DWARFFormValue> sizeResult = type_die.find(DW_AT_byte_size);
  if (!sizeResult) {
    llvm::errs() << "Failed to get size of variable.\n";
    return std::nullopt;
  }
  return sizeResult->getAsUnsignedConstant().value();
}
// Get sections (.bss and .data)
bool loadSections(const std::string &binary_path, std::vector<SectionInfo> &sections) {
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> buffer_or_err =
      llvm::MemoryBuffer::getFile(binary_path);
  if (std::error_code ec = buffer_or_err.getError()) {
    llvm::errs() << "Error reading file: " << ec.message() << "\n";
    return false;
  }

  llvm::Expected<std::unique_ptr<llvm::object::ObjectFile>> obj_or_err =
      llvm::object::ObjectFile::createObjectFile(buffer_or_err.get()->getMemBufferRef());
  if (!obj_or_err) {
    llvm::errs() << "Failed to create ObjectFile.\n";
    return false;
  }

  llvm::object::ObjectFile *obj = obj_or_err->get();

  for (const auto &section : obj->sections()) {
    llvm::StringRef name = section.getName().get();
    if (name == ".data" || name == ".bss") {
      llvm::ErrorOr<uint64_t> addr_or_err = section.getAddress();
      llvm::ErrorOr<uint64_t> size_or_err = section.getSize();
      if (addr_or_err && size_or_err) {
        SectionInfo sec;
        sec.name = name.str();
        sec.start_address = *addr_or_err;
        sec.end_address = sec.start_address + *size_or_err;
        sections.push_back(sec);
        llvm::outs() << "Loaded section " << sec.name
                     << " with start address: " << llvm::format_hex(sec.start_address, 16)
                     << " and end address: " << llvm::format_hex(sec.end_address, 16) << "\n";
      } else {
        llvm::errs() << "Failed to get address or size for section: " << name << "\n";
      }
    }
  }

  if (sections.empty()) {
    llvm::errs() << "No .data or .bss sections found.\n";
    return false;
  }

  return true;
}

// Function to find the section containing a given address
std::optional<SectionInfo> findSection(uint64_t address, const std::vector<SectionInfo> &sections) {
  for (const auto &sec : sections) {
    if (address >= sec.start_address && address < sec.end_address) {
      return sec;
    }
  }
  return std::nullopt;
}  // Function to load .debug_addr section
bool loadDebugAddr(const std::string &binary_path, std::vector<uint64_t> &addr_table) {
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> buffer_or_err =
      llvm::MemoryBuffer::getFile(binary_path);
  if (std::error_code ec = buffer_or_err.getError()) {
    llvm::errs() << "Error reading file: " << ec.message() << "\n";
    return false;
  }

  llvm::Expected<std::unique_ptr<llvm::object::ObjectFile>> obj_or_err =
      llvm::object::ObjectFile::createObjectFile(buffer_or_err.get()->getMemBufferRef());
  if (!obj_or_err) {
    llvm::errs() << "Failed to create ObjectFile.\n";
    return false;
  }

  llvm::object::ObjectFile *obj = obj_or_err->get();

  for (const auto &section : obj->sections()) {
    llvm::StringRef name = section.getName().get();
    if (name == ".debug_addr") {
      llvm::StringRef contents = section.getContents().get();
      llvm::DWARFDataExtractor data(contents, true, 8);
      uint64_t offset = 0;

      // Header is 12 bytes (4 bytes for length, 2 bytes for version, 1 byte for addr_size and
      // seg_size)
      if (contents.size() < 12) {
        llvm::errs() << ".debug_addr section too small.\n";
        return false;
      }

      // Offset is automatically shifted by getFN
      uint32_t length = data.getU32(&offset);
      uint16_t version = data.getU16(&offset);
      uint8_t addr_size = data.getU8(&offset);
      uint8_t seg_size = data.getU8(&offset);

      llvm::outs() << "Loaded .debug_addr header: Length=" << llvm::format_hex(length, 8)
                   << ", Version=" << llvm::format_hex(version, 4)
                   << ", Addr Size=" << static_cast<int>(addr_size)
                   << ", Seg Size=" << static_cast<int>(seg_size) << "\n";

      // Read addresses
      while (offset < contents.size()) {
        uint64_t addr = data.getAddress(&offset);
        addr_table.push_back(addr);
      }

      llvm::outs() << "Loaded .debug_addr with " << addr_table.size() << " entries.\n";
      return true;
    }
  }

  llvm::errs() << ".debug_addr section not found.\n";
  return false;
}

// Function to retrieve and parse struct members
bool getStructMembers(const llvm::DWARFDie &type_die, std::vector<StructMemberInfo> &members,
                      const llvm::DWARFContext &context) {
  if (type_die.getTag() != DW_TAG_structure_type && type_die.getTag() != DW_TAG_class_type) {
    llvm::errs() << "Type is not a struct or class.\n";
    return false;
  }

  for (const DWARFDie &member_die : type_die.children()) {
    if (member_die.getTag() == DW_TAG_member) {
      std::optional<DWARFFormValue> nameResult = member_die.find(DW_AT_name);
      std::optional<DWARFFormValue> offsetResult = member_die.find(DW_AT_data_member_location);
      // Cannot resolve size if it is a type again - so lets resolve it now
      std::optional<uint64_t> sizeResult = findSizeOfVariable(member_die);

      // Usually the name is enough for validity
      if (nameResult) {
        auto member_name = nameResult->getAsCString().get();
        uint64_t member_offset = offsetResult->getAsUnsignedConstant().value();
        uint64_t member_size = sizeResult ? sizeResult.value() : 0;

        StructMemberInfo member;
        member.name = member_name;
        member.offset = member_offset;
        member.size = member_size;
        members.push_back(member);

        llvm::outs() << "----> Member: " << member.name << ", Offset: 0x"
                     << llvm::format_hex(member.offset, 8) << ", Size: " << member.size
                     << " bytes\n";
      }
    }
  }

  if (members.empty()) {
    llvm::errs() << "No members found for struct/class.\n";
    return false;
  }

  return true;
}

// Recursive function to resolve fully qualified field name based on offset and type DIE
std::optional<std::string> getFullyQualifiedFieldName(uint64_t offset,
                                                      const llvm::DWARFDie &type_die,
                                                      const llvm::DWARFContext &context,
                                                      std::vector<std::string> &path) {
  std::string qualified_name;

  if (type_die.getTag() == DW_TAG_pointer_type) {
    path.push_back("->");
    // Get the type of the member
    std::optional<DWARFFormValue> member_type_result = type_die.find(DW_AT_type);
    llvm::DWARFDie member_type_die =
        type_die.getDwarfUnit()->getDIEForOffset(*member_type_result->getAsReference());

    return getFullyQualifiedFieldName(offset, member_type_die, context, path);
  }

  // Primitive types
  if (type_die.getTag() == DW_TAG_base_type || type_die.getTag() == DW_TAG_enumeration_type ||
      type_die.getTag() == DW_TAG_union_type || type_die.getTag() == DW_TAG_array_type) {
    std::string name;

    std::optional<llvm::StringRef> prim_name = type_die.getName(DINameKind::ShortName);
    name = prim_name ? prim_name->str() : "[unk primitive]";

    for (size_t i = 0; i < path.size(); ++i) {
      if (i != 0) qualified_name += ".";
      qualified_name += path[i];
    }
    qualified_name += "(" + name + ")";
    return qualified_name;
  }

  // Ensure the type is a struct or class
  if (type_die.getTag() != DW_TAG_structure_type && type_die.getTag() != DW_TAG_class_type)
    return std::nullopt;

  for (const DWARFDie &member_die : type_die.children()) {
    if (member_die.getTag() == DW_TAG_member) {
      // Get type information for the member
      std::optional<DWARFFormValue> nameResult = member_die.find(DW_AT_name);
      std::optional<DWARFFormValue> offsetResult = member_die.find(DW_AT_data_member_location);
      std::optional<uint64_t> sizeResult = findSizeOfVariable(member_die);

      if (nameResult && offsetResult && sizeResult) {
        auto member_name = nameResult->getAsCString().get();
        uint64_t member_offset = offsetResult->getAsUnsignedConstant().value();
        uint64_t member_size = sizeResult ? sizeResult.value() : 0;

        llvm::outs() << " ---> Member: " << member_name << ", Offset: 0x"
                     << llvm::format_hex(member_offset, 10) << ", Size: " << member_size
                     << " bytes\n";

        if (offset >= member_offset && offset < (member_offset + member_size)) {
          path.push_back(member_name);

          // Get the type of the member
          std::optional<DWARFFormValue> member_type_result = member_die.find(DW_AT_type);

          if (member_type_result) {
            llvm::DWARFDie member_type_die =
                member_die.getDwarfUnit()->getDIEForOffset(*member_type_result->getAsReference());
            if (!member_type_die) {
              llvm::errs() << "Failed to resolve type DIE of member!\n";
            }
            // Check if the member is itself a struct or class for nested types
            if (member_type_die.getTag() == DW_TAG_structure_type ||
                member_type_die.getTag() == DW_TAG_class_type ||
                member_type_die.getTag() == DW_TAG_union_type ||
                member_type_die.getTag() == DW_TAG_pointer_type ||
                member_type_die.getTag() == DW_TAG_enumeration_type ||
                member_type_die.getTag() == DW_TAG_base_type) {
              uint64_t new_offset = offset - member_offset;
              return getFullyQualifiedFieldName(new_offset, member_type_die, context, path);
            }
          }

          // else if (member_type_die.getTag() == dwarf::DW_TAG_enumeration_type) {
          // Enums are "leaves" (also considered "primitive")
          // Base case: member is a primitive type
          // Construct the fully qualified name
          std::string qualified_name;
          for (size_t i = 0; i < path.size(); ++i) {
            if (i != 0) qualified_name += ".";
            qualified_name += path[i];
          }
          return qualified_name;
        }
      }
    }
  }

  return std::nullopt;
}

// https://dwarfstd.org/dwarf5std.html
// The DWARF v5 standard uses a new DW_OP_addrx operation for addresses in the .debug_addr section
// So they have to be resolved beforehand (cf. page 45 for DWARFv5)
std::optional<int> evaluateAddrxDWARFExpression(const llvm::DWARFExpression &Expression,
                                                uint64_t AddressSize,
                                                const std::vector<uint64_t> &addr_table) {
  std::stack<int64_t> OperandStack;

  for (auto it = Expression.begin(); it != Expression.end(); ++it) {
    const llvm::DWARFExpression::Operation &Op = *it;

    switch (Op.getCode()) {
      case DW_OP_consts: {
        if (Op.getNumOperands() != 1) {
          llvm::errs() << "DW_OP_consts requires exactly 1 operand.\n";
          return -1;
        }
        int64_t value = Op.getRawOperand(0);
        OperandStack.push(value);
        // llvm::outs() << "DW_OP_consts: Pushed " << value << "\n";
        break;
      }
      case DW_OP_plus: {
        if (OperandStack.size() < 2) {
          llvm::errs() << "Insufficient operands for DW_OP_plus\n";
          return -1;
        }
        int64_t a = OperandStack.top();
        OperandStack.pop();
        int64_t b = OperandStack.top();
        OperandStack.pop();
        OperandStack.push(a + b);
        // llvm::outs() << "DW_OP_plus: Popped " << b << " and " << a << ", pushed " << (a + b) <<
        // "\n";
        break;
      }
      case DW_OP_addrx: {
        if (Op.getNumOperands() != 1) {
          llvm::errs() << "DW_OP_addrx requires exactly 1 operand.\n";
          return -1;
        }
        int64_t index = Op.getRawOperand(0);
        if (index < 0 || static_cast<size_t>(index) >= addr_table.size()) {
          llvm::errs() << "DW_OP_addrx: Index " << index << " out of bounds.\n";
          return -1;
        }
        uint64_t address = addr_table[index];
        OperandStack.push(static_cast<int64_t>(address));
        // llvm::outs() << "DW_OP_addrx: Pushed address 0x" << llvm::format_hex(address, 16) <<
        // "\n";
        break;
      }
      // Handle other opcodes as needed
      default:
        return std::nullopt;
    }
  }

  if (OperandStack.empty()) {
    llvm::errs() << "Operand stack is empty after evaluation.\n";
    return std::nullopt;
  }

  return OperandStack.top();
}

// Function to find the fully qualified field name for a given address offset
std::optional<std::string> findFullyQualifiedField(uint64_t address_offset,
                                                   const llvm::DWARFDie &variable_die,
                                                   const llvm::DWARFContext &context) {
  // Retrieve the type of the variable
  std::optional<DWARFFormValue> typeResult = variable_die.find(DW_AT_type);
  if (!typeResult) {
    llvm::errs() << "Variable has no type information.\n";
    return std::nullopt;
  }

  // Figuring the api out is a bit painful
  llvm::DWARFDie type_die =
      variable_die.getDwarfUnit()->getDIEForOffset(*typeResult->getAsReference());
  if (!type_die) {
    llvm::errs() << "Failed to resolve type DIE of variable!\n";
    return std::nullopt;
  }

  std::vector<std::string> path;
  std::optional<std::string> field_name =
      getFullyQualifiedFieldName(address_offset, type_die, context, path);

  if (!field_name) {
    llvm::errs() << "Failed to get field name.\n";
    return std::nullopt;
  }

  return field_name;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
  llvm:
    errs() << "Usage: " << argv[0] << " <binary_path> <address_in_hex>" << "\n";
    return 1;
  }

  std::string binary_path = argv[1];
  uintptr_t target_address = strtoul(argv[2], nullptr, 16);

  llvm::outs() << "Looking for target address: " << llvm::format_hex(target_address, 10)
               << " in binary " << binary_path << "\n";

  std::vector<SectionInfo> sections;
  std::vector<uint64_t> addr_table;

  // Load relevant sections (.data and .bss)
  if (!loadSections(binary_path, sections)) {
    return 1;
  }

  // Load .debug_addr section
  if (!loadDebugAddr(binary_path, addr_table)) {
    return 1;
  }

  // Initialize DWARFContext
  auto binary_or_err = llvm::MemoryBuffer::getFile(binary_path);
  if (std::error_code ec = binary_or_err.getError()) {
    llvm::errs() << "Error reading file: " << ec.message() << "\n";
    return 1;
  }

  auto obj_or_err =
      llvm::object::ObjectFile::createObjectFile(binary_or_err.get()->getMemBufferRef());
  if (!obj_or_err) {
    llvm::errs() << "Failed to create ObjectFile.\n";
    return 1;
  }

  std::unique_ptr<llvm::DWARFContext> dwarf_context = llvm::DWARFContext::create(*obj_or_err.get());

  // Iterate over Compilation Units
  for (const auto &cu : dwarf_context->compile_units()) {
    DWARFUnit *CU = cu.get();

    // Go over global variables of a CU
    for (const llvm::DWARFDebugInfoEntry &die_entry : CU->dies()) {
      llvm::DWARFDie member_die = llvm::DWARFDie(CU, &die_entry);

      if (member_die.getTag() == DW_TAG_variable) {
        // Get the location of the variable (DW_AT_location)
        std::optional<DWARFFormValue> locationResult = member_die.find(DW_AT_location);
        if (locationResult) {
          Form form = locationResult->getForm();

          if (locationResult->getForm() == DW_FORM_data4 ||
              locationResult->getForm() == DW_FORM_data8) {
            uint64_t address = locationResult->getAsUnsignedConstant().value();
            llvm::outs() << "Location (address): 0x" << address << "\n";

            // Determine the section and calculate the offset
            std::optional<SectionInfo> section_opt = findSection(address, sections);
            if (section_opt) {
              uint64_t offset = address - section_opt->start_address;
              std::optional<uint64_t> size = findSizeOfVariable(member_die);

              if (!size || offset + size.value() < target_address || offset > target_address) {
                // Skip if the target address is not within the variable
                continue;
              }

              llvm::outs() << "Section: " << section_opt->name << ", Offset: 0x"
                           << llvm::format_hex(offset, /*Width=*/16) << "Size: " << size.value()
                           << "\n";

              // Find the fully qualified field name
              std::optional<std::string> field_name =
                  findFullyQualifiedField(offset, member_die, *dwarf_context);
              if (field_name) {
                llvm::outs() << "Fully Qualified Field Name: " << field_name.value()
                             << ", Size: " << size.value() << "\n";

              } else {
                llvm::outs() << "Could not determine the fully qualified field name.\n";
              }
            } else {
              llvm::outs() << "Address " << format_hex(address, 10)
                           << " not found in known sections.\n";
            }
          } else if (form == DW_FORM_exprloc) {
            // llvm::outs() << "Expresssion Location Evaluation" << "\n";
            auto Location = locationResult->getAsBlock();
            if (Location) {
              // Parse the DWARF expression that encodes the struct position
              llvm::DataExtractor Extractor(
                  llvm::StringRef((const char *)Location->data(), Location->size()),
                  CU->getAddressByteSize(), CU->isLittleEndian());

              llvm::DWARFExpression Expression(Extractor, CU->getAddressByteSize());

              // Evaluate the expression
              std::optional<int> evaluated_address_opt =
                  evaluateAddrxDWARFExpression(Expression, CU->getAddressByteSize(), addr_table);
              if (!evaluated_address_opt) {
                // llvm::outs() << "Unsupported DWARF expression.\n";
                continue;
              }

              // If the var is global and not nameed, it is compiler generated
              std::optional<DWARFFormValue> nameResult = member_die.find(DW_AT_name);
              char *name;
              if (nameResult) {
                auto cname = nameResult->getAsCString();
                name = (char *)cname.get();
                llvm::outs() << "----> Global Variable: " << name << "\n";
              } else {
                // llvm::outs() << "Skipping global anonymous variable....\n";
                continue;
              }

              int evaluated_address = evaluated_address_opt.value();
              llvm::outs() << "Evaluated Address: " << format_hex(evaluated_address, 10) << "\n";

              // Determine the section and calculate the offset
              std::optional<SectionInfo> section_opt =
                  findSection(static_cast<uint64_t>(evaluated_address), sections);
              if (section_opt) {
                uint64_t offset =
                    static_cast<uint64_t>(evaluated_address) - section_opt->start_address;

                std::optional<uint64_t> size = findSizeOfVariable(member_die);

                if (!size || offset + size.value() < target_address || offset > target_address) {
                  // Skip if the target address is not within the variable
                  continue;
                }

                llvm::outs() << "Section: " << section_opt->name << ", Offset: 0x"
                             << llvm::format_hex(offset, /*Width=*/16) << ", Size: " << size.value()
                             << "\n";

                // Target address is within the variable: so target_address - offset > 0  && < size
                std::optional<std::string> field_name =
                    findFullyQualifiedField(target_address - offset, member_die, *dwarf_context);
                if (field_name) {
                  llvm::outs() << "Fully Qualified Field Name: " << name << "."
                               << field_name.value() << ", In Variable of Size: " << size.value()
                               << "\n";
                  return 0;

                } else {
                  llvm::outs() << "Could not determine the fully qualified field name.\n";
                }
              } else {
                llvm::outs() << "Evaluated address 0x" << evaluated_address
                             << " not found in known sections.\n";
              }
            }
          }
        }
      }
    }
  }

  llvm::outs() << "Target address " << target_address << " not found in any global variable.\n";
  return 0;
}
