#pragma once
#include <Windows.h>
#include <algorithm>
#include <vmprofiler.hpp>
#include <xtils.hpp>
#include <parser.hpp>

namespace vm {
/// <summary>
/// struct containing encoded data for a given virtual instruction...
/// </summary>
struct vinstr_data {
  /// <summary>
  /// vm handler index also known as the opcode...
  /// </summary>
  std::uint8_t vm_handler;

  /// <summary>
  /// this field contains the second operand if any...
  /// </summary>
  std::uint64_t operand;

  /// <summary>
  /// size in bits of the second operand if any... zero if none...
  /// </summary>
  std::uint8_t imm_size;
};

/// <summary>
/// struct containing all information for a label...
/// </summary>
struct vlabel_data {
  /// <summary>
  /// name of the label...
  /// </summary>
  std::string label_name;

  /// <summary>
  /// vector of encoded virtual instructions...
  /// </summary>
  std::vector<vinstr_data> vinstrs;
};

/// <summary>
/// struct containing compiled virtual instructions (encoded and encrypted) for
/// a given label...
/// </summary>
struct compiled_label_data {
  /// <summary>
  /// label name...
  /// </summary>
  std::string label_name;

  /// <summary>
  /// relative virtual address from vm_entry to the virtual instructions...
  /// </summary>
  std::uintptr_t alloc_rva;

  /// <summary>
  /// encrypted relative virtual address from vm_entry to virtual
  /// instructions...
  /// </summary>
  std::uintptr_t enc_alloc_rva;

  /// <summary>
  /// vector of bytes containing the raw, encrypted virtual instructions...
  /// </summary>
  std::vector<std::uint8_t> vinstrs;
};

/// <summary>
/// class containing member functions used to encode and encrypted virtual
/// instructions...
/// </summary>
class compiler_t {
 public:
  /// <summary>
  /// default constructor
  /// </summary>
  /// <param name="vmctx">pointer to a vm context object which has already been
  /// init...</param>
  explicit compiler_t(vm::ctx_t* vmctx);

  /// <summary>
  /// encode virtual instructions from parser::virt_labels
  /// </summary>
  /// <returns>returns a vector of labels containing encoded virtual
  /// instructions</returns>
  std::vector<vlabel_data>* encode();

  /// <summary>
  /// encrypt virtual instructions from parser::virt_labels
  /// </summary>
  /// <returns>returns a vector of compiled labels containing encoded and
  /// encrypted virtual instructions...</returns>
  std::vector<compiled_label_data> encrypt();

 private:
  /// <summary>
  /// encrypt virtual instructions rva... <a
  /// href="https://back.engineering/17/05/2021/#vm_entry">read more here...</a>
  /// </summary>
  /// <param name="rva">relative virtual address to encrypted virtual
  /// instructions...</param> <returns></returns>
  std::uint64_t encrypt_rva(std::uint64_t rva);

  /// <summary>
  /// pointer to the vmctx passed in by the constructor...
  /// </summary>
  vm::ctx_t* vmctx;

  /// <summary>
  /// transformations used to decrypt the opcode operand extracted from
  /// calc_jmp... you can read more <a
  /// href="https://back.engineering/17/05/2021/#calc_jmp">here...</a>
  /// </summary>
  transform::map_t calc_jmp_transforms;

  /// <summary>
  /// vector of encoded labels...
  /// </summary>
  std::vector<vlabel_data> virt_labels;

  /// <summary>
  /// vector of decoded zydis instructions containing the native instructions to
  /// encrypt the virtual instruction rva which will be pushed onto the stack
  /// prior to jmping to vm entry...
  /// </summary>
  std::vector<zydis_decoded_instr_t> encrypt_vinstrs_rva;
};
}  // namespace vm