#pragma once
#include <unicorn/unicorn.h>

#include <atomic>
#include <nt/image.hpp>
#include <vmprofiler.hpp>

#define PAGE_4KB 0x1000
#define STACK_SIZE PAGE_4KB * 512

#define STACK_BASE 0xFFFF000000000000
#define IAT_VECTOR_TABLE 0xFFFFF00000000000

namespace vm {
inline bool g_force_emu = false;

class emu_t {
  struct cpu_ctx_t {
    std::uintptr_t rip;
    uc_context* context;
    std::uint8_t stack[STACK_SIZE];
  };

  struct code_block_data_t {
    vm::instrs::code_block_t code_block;
    std::shared_ptr<cpu_ctx_t> cpu_ctx;
    std::shared_ptr<vm::ctx_t> g_vm_ctx;
  };

 public:
  explicit emu_t(vm::ctx_t* vm_ctx);
  ~emu_t();

  bool init();
  bool get_trace(std::vector<vm::instrs::code_block_t>& code_blocks);

 private:
  std::uintptr_t img_base, img_size;
  uc_hook code_exec_hook, invalid_mem_hook, int_hook;

  uc_engine* uc_ctx;
  vm::ctx_t* g_vm_ctx;
  code_block_data_t* cc_block;

  std::vector<std::uintptr_t> vip_begins;
  std::vector<code_block_data_t> code_blocks;
  std::map<std::uintptr_t, std::shared_ptr<vm::ctx_t> > vm_ctxs;

  uc_err create_entry(vmp2::v2::entry_t* entry);
  static void int_callback(uc_engine* uc, std::uint32_t intno, emu_t* obj);
  static bool code_exec_callback(uc_engine* uc,
                                 uint64_t address,
                                 uint32_t size,
                                 emu_t* obj);

  static void invalid_mem(uc_engine* uc,
                          uc_mem_type type,
                          uint64_t address,
                          int size,
                          int64_t value,
                          emu_t* obj);
};
}  // namespace vm