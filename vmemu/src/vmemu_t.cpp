#include "vmemu_t.hpp"

namespace vm {
emu_t::emu_t(vm::ctx_t* vm_ctx)
    : g_vm_ctx(vm_ctx),
      uc_ctx(nullptr),
      img_base(vm_ctx->image_base),
      img_size(vm_ctx->image_size) {}

emu_t::~emu_t() {
  if (uc_ctx) uc_close(uc_ctx);
}

bool emu_t::init() {
  uc_err err;
  if ((err = uc_open(UC_ARCH_X86, UC_MODE_64, &uc_ctx))) {
    std::printf("> uc_open err = %d\n", err);
    return false;
  }

  if ((err = uc_mem_map(uc_ctx, STACK_BASE, STACK_SIZE, UC_PROT_ALL))) {
    std::printf("> uc_mem_map stack err, reason = %d\n", err);
    return false;
  }

  if ((err = uc_mem_map(uc_ctx, IAT_VECTOR_TABLE, PAGE_4KB, UC_PROT_ALL))) {
    std::printf("> uc_mem_map iat vector table err = %d\n", err);
    return false;
  }

  // init iat vector table full of 'ret' instructions...
  auto c3_page = malloc(PAGE_4KB);
  {
    memset(c3_page, 0xC3, PAGE_4KB);

    if ((err = uc_mem_write(uc_ctx, IAT_VECTOR_TABLE, c3_page, PAGE_4KB))) {
      std::printf("> failed to init iat vector table...\n");
      free(c3_page);
      return false;
    }
  }
  free(c3_page);

  auto win_img = reinterpret_cast<win::image_t<>*>(g_vm_ctx->module_base);

  // iat hook all imports to return...
  for (auto import_dir = reinterpret_cast<win::import_directory_t*>(
           win_img->get_directory(win::directory_id::directory_entry_import)
               ->rva +
           g_vm_ctx->module_base);
       import_dir->rva_name; ++import_dir) {
    for (auto iat_thunk = reinterpret_cast<win::image_thunk_data_t<>*>(
             import_dir->rva_first_thunk + g_vm_ctx->module_base);
         iat_thunk->address; ++iat_thunk) {
      if (iat_thunk->is_ordinal) continue;
      iat_thunk->function = IAT_VECTOR_TABLE;
    }
  }

  if ((err =
           uc_mem_map(uc_ctx, g_vm_ctx->module_base, img_size, UC_PROT_ALL))) {
    std::printf("> map memory failed, reason = %d\n", err);
    return false;
  }

  if ((err = uc_mem_write(uc_ctx, g_vm_ctx->module_base,
                          reinterpret_cast<void*>(g_vm_ctx->module_base),
                          img_size))) {
    std::printf("> failed to write memory... reason = %d\n", err);
    return false;
  }

  if ((err = uc_hook_add(uc_ctx, &code_exec_hook, UC_HOOK_CODE,
                         (void*)&vm::emu_t::code_exec_callback, this,
                         g_vm_ctx->module_base,
                         g_vm_ctx->module_base + img_size))) {
    std::printf("> uc_hook_add error, reason = %d\n", err);
    return false;
  }

  if ((err = uc_hook_add(uc_ctx, &int_hook, UC_HOOK_INTR,
                         (void*)&vm::emu_t::int_callback, this, 0ull, 0ull))) {
    std::printf("> uc_hook_add error, reason = %d\n", err);
    return false;
  }

  if ((err =
           uc_hook_add(uc_ctx, &invalid_mem_hook,
                       UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_WRITE_UNMAPPED |
                           UC_HOOK_MEM_FETCH_UNMAPPED,
                       (void*)&vm::emu_t::invalid_mem, this, true, false))) {
    std::printf("> uc_hook_add error, reason = %d\n", err);
    return false;
  }
  return true;
}

bool emu_t::get_trace(std::vector<vm::instrs::code_block_t>& entries) {
  uc_err err;
  std::uintptr_t rip = g_vm_ctx->vm_entry_rva + g_vm_ctx->module_base,
                 rsp = STACK_BASE + STACK_SIZE - PAGE_4KB;

  if ((err = uc_reg_write(uc_ctx, UC_X86_REG_RSP, &rsp))) {
    std::printf("> uc_reg_write error, reason = %d\n", err);
    return false;
  }

  if ((err = uc_reg_write(uc_ctx, UC_X86_REG_RIP, &rip))) {
    std::printf("> uc_reg_write error, reason = %d\n", err);
    return false;
  }

  // trace the first block given the vm enter...
  code_block_data_t code_block{{}, nullptr, nullptr};
  cc_block = &code_block;

  std::printf("> beginning execution at = %p\n", rip);
  if ((err = uc_emu_start(uc_ctx, rip, 0ull, 0ull, 0ull))) {
    std::printf("> error starting emu... reason = %d\n", err);
    return false;
  }

  if (cc_block) code_blocks.push_back(code_block);

  // code_blocks.size() will continue to grow as all branches are traced...
  // when idx is > code_blocks.size() then we have traced all branches...
  for (auto idx = 0u; idx < code_blocks.size(); ++idx) {
    const auto _code_block = code_blocks[idx];
    if (!_code_block.code_block.jcc.has_jcc) continue;

    switch (_code_block.code_block.jcc.type) {
      case vm::instrs::jcc_type::branching: {
        if (std::find(vip_begins.begin(), vip_begins.end(),
                      _code_block.code_block.jcc.block_addr[1]) ==
            vip_begins.end()) {
          std::uintptr_t rbp = 0ull;
          std::uint32_t branch_rva = (_code_block.code_block.jcc.block_addr[1] -
                                      g_vm_ctx->module_base) +
                                     g_vm_ctx->image_base;

          // setup object globals so that the tracing will work...
          code_block_data_t branch_block{{}, nullptr, nullptr};
          cc_block = &branch_block;
          g_vm_ctx = _code_block.g_vm_ctx.get();

          // restore register values...
          if ((err =
                   uc_context_restore(uc_ctx, _code_block.cpu_ctx->context))) {
            std::printf("> failed to restore emu context... reason = %d\n",
                        err);
            return false;
          }

          // restore stack values...
          if ((err = uc_mem_write(uc_ctx, STACK_BASE,
                                  _code_block.cpu_ctx->stack, STACK_SIZE))) {
            std::printf("> failed to restore stack... reason = %d\n", err);
            return false;
          }

          // get the address in rbp (top of vsp)... then patch the branch rva...
          if ((err = uc_reg_read(uc_ctx, UC_X86_REG_RBP, &rbp))) {
            std::printf("> failed to read rbp... reason = %d\n", err);
            return false;
          }

          // patch the branch rva...
          if ((err =
                   uc_mem_write(uc_ctx, rbp, &branch_rva, sizeof branch_rva))) {
            std::printf("> failed to patch branch rva... reason = %d\n", err);
            return false;
          }

          std::printf("> beginning execution at = %p\n",
                      _code_block.cpu_ctx->rip);
          if ((err = uc_emu_start(uc_ctx, _code_block.cpu_ctx->rip, 0ull, 0ull,
                                  0ull))) {
            std::printf("> error starting emu... reason = %d\n", err);
            return false;
          }

          if (cc_block)
            // push back new block that has been traced...
            code_blocks.push_back(branch_block);
        }
        // drop down and execute the absolute case as well since that
        // will trace the first branch...
      }
      case vm::instrs::jcc_type::absolute: {
        if (std::find(vip_begins.begin(), vip_begins.end(),
                      _code_block.code_block.jcc.block_addr[0]) ==
            vip_begins.end()) {
          std::uintptr_t rbp = 0ull;
          std::uint32_t branch_rva = (_code_block.code_block.jcc.block_addr[0] -
                                      g_vm_ctx->module_base) +
                                     g_vm_ctx->image_base;

          // setup object globals so that the tracing will work...
          code_block_data_t branch_block{{}, nullptr, nullptr};
          cc_block = &branch_block;
          g_vm_ctx = _code_block.g_vm_ctx.get();

          // restore register values...
          if ((err =
                   uc_context_restore(uc_ctx, _code_block.cpu_ctx->context))) {
            std::printf("> failed to restore emu context... reason = %d\n",
                        err);
            return false;
          }

          // restore stack values...
          if ((err = uc_mem_write(uc_ctx, STACK_BASE,
                                  _code_block.cpu_ctx->stack, STACK_SIZE))) {
            std::printf("> failed to restore stack... reason = %d\n", err);
            return false;
          }

          // get the address in rbp (top of vsp)... then patch the branch rva...
          if ((err = uc_reg_read(uc_ctx, UC_X86_REG_RBP, &rbp))) {
            std::printf("> failed to read rbp... reason = %d\n", err);
            return false;
          }

          // patch the branch rva...
          if ((err =
                   uc_mem_write(uc_ctx, rbp, &branch_rva, sizeof branch_rva))) {
            std::printf("> failed to patch branch rva... reason = %d\n", err);
            return false;
          }

          std::printf("> beginning execution at = %p\n",
                      _code_block.cpu_ctx->rip);
          if ((err = uc_emu_start(uc_ctx, _code_block.cpu_ctx->rip, 0ull, 0ull,
                                  0ull))) {
            std::printf("> error starting emu... reason = %d\n", err);
            return false;
          }

          if (cc_block)
            // push back new block that has been traced...
            code_blocks.push_back(branch_block);
        }
        break;
      }
      case vm::instrs::jcc_type::switch_case: {
        for (auto _idx = 0u;
             _idx < _code_block.code_block.jcc.block_addr.size(); ++_idx) {
          if (std::find(vip_begins.begin(), vip_begins.end(),
                        _code_block.code_block.jcc.block_addr[_idx]) !=
              vip_begins.end())
            continue;

          std::uintptr_t rbp = 0ull;
          std::uint32_t branch_rva =
              (_code_block.code_block.jcc.block_addr[_idx] -
               g_vm_ctx->module_base) +
              g_vm_ctx->image_base;

          // setup object globals so that the tracing will work...
          code_block_data_t branch_block{{}, nullptr, nullptr};
          cc_block = &branch_block;
          g_vm_ctx = _code_block.g_vm_ctx.get();

          // restore register values...
          if ((err =
                   uc_context_restore(uc_ctx, _code_block.cpu_ctx->context))) {
            std::printf("> failed to restore emu context... reason = %d\n",
                        err);
            return false;
          }

          // restore stack values...
          if ((err = uc_mem_write(uc_ctx, STACK_BASE,
                                  _code_block.cpu_ctx->stack, STACK_SIZE))) {
            std::printf("> failed to restore stack... reason = %d\n", err);
            return false;
          }

          // get the address in rbp (top of vsp)... then patch the branch rva...
          if ((err = uc_reg_read(uc_ctx, UC_X86_REG_RBP, &rbp))) {
            std::printf("> failed to read rbp... reason = %d\n", err);
            return false;
          }

          // patch the branch rva...
          if ((err =
                   uc_mem_write(uc_ctx, rbp, &branch_rva, sizeof branch_rva))) {
            std::printf("> failed to patch branch rva... reason = %d\n", err);
            return false;
          }

          std::printf("> beginning execution at = %p\n",
                      _code_block.cpu_ctx->rip);
          if ((err = uc_emu_start(uc_ctx, _code_block.cpu_ctx->rip, 0ull, 0ull,
                                  0ull))) {
            std::printf("> error starting emu... reason = %d\n", err);
            return false;
          }

          if (cc_block)
            // push back new block that has been traced...
            code_blocks.push_back(branch_block);
        }
        break;
      }
    }
  }

  for (auto& [code_block, cpu_ctx, vm_ctx] : code_blocks) {
    // convert linear virtual addresses to image based addresses...
    code_block.vip_begin =
        (code_block.vip_begin - g_vm_ctx->module_base) + g_vm_ctx->image_base;
    if (code_block.jcc.has_jcc) {
      switch (code_block.jcc.type) {
        case vm::instrs::jcc_type::branching: {
          code_block.jcc.block_addr[1] =
              (code_block.jcc.block_addr[1] - g_vm_ctx->module_base) +
              g_vm_ctx->image_base;
        }
        case vm::instrs::jcc_type::absolute: {
          code_block.jcc.block_addr[0] =
              (code_block.jcc.block_addr[0] - g_vm_ctx->module_base) +
              g_vm_ctx->image_base;
          break;
        }
        case vm::instrs::jcc_type::switch_case: {
          for (auto idx = 0u; idx < code_block.jcc.block_addr.size(); ++idx)
            code_block.jcc.block_addr[idx] =
                (code_block.jcc.block_addr[idx] - g_vm_ctx->module_base) +
                g_vm_ctx->image_base;
          break;
        }
      }
    }
    entries.push_back(code_block);
  }

  return true;
}

uc_err emu_t::create_entry(vmp2::v2::entry_t* entry) {
  uc_reg_read(uc_ctx, UC_X86_REG_R15, &entry->regs.r15);
  uc_reg_read(uc_ctx, UC_X86_REG_R14, &entry->regs.r14);
  uc_reg_read(uc_ctx, UC_X86_REG_R13, &entry->regs.r13);
  uc_reg_read(uc_ctx, UC_X86_REG_R12, &entry->regs.r12);
  uc_reg_read(uc_ctx, UC_X86_REG_R11, &entry->regs.r11);
  uc_reg_read(uc_ctx, UC_X86_REG_R10, &entry->regs.r10);
  uc_reg_read(uc_ctx, UC_X86_REG_R9, &entry->regs.r9);
  uc_reg_read(uc_ctx, UC_X86_REG_R8, &entry->regs.r8);
  uc_reg_read(uc_ctx, UC_X86_REG_RBP, &entry->regs.rbp);
  uc_reg_read(uc_ctx, UC_X86_REG_RDI, &entry->regs.rdi);
  uc_reg_read(uc_ctx, UC_X86_REG_RSI, &entry->regs.rsi);
  uc_reg_read(uc_ctx, UC_X86_REG_RDX, &entry->regs.rdx);
  uc_reg_read(uc_ctx, UC_X86_REG_RCX, &entry->regs.rcx);
  uc_reg_read(uc_ctx, UC_X86_REG_RBX, &entry->regs.rbx);
  uc_reg_read(uc_ctx, UC_X86_REG_RAX, &entry->regs.rax);
  uc_reg_read(uc_ctx, UC_X86_REG_EFLAGS, &entry->regs.rflags);

  entry->vip = entry->regs.rsi;
  entry->handler_idx = entry->regs.rax;
  entry->decrypt_key = entry->regs.rbx;

  uc_err err;
  if ((err = uc_mem_read(uc_ctx, entry->regs.rdi, entry->vregs.raw,
                         sizeof entry->vregs.raw)))
    return err;

  // copy virtual stack values...
  for (auto idx = 0u; idx < sizeof(entry->vsp) / 8; ++idx)
    if ((err =
             uc_mem_read(uc_ctx, entry->regs.rbp + (idx * 8),
                         &entry->vsp.qword[idx], sizeof entry->vsp.qword[idx])))
      return err;

  return UC_ERR_OK;
}

bool emu_t::code_exec_callback(uc_engine* uc, uint64_t address, uint32_t size,
                               emu_t* obj) {
  uc_err err;
  vmp2::v2::entry_t vinstr_entry;
  std::uint8_t vm_handler_table_idx = 0u;
  std::uintptr_t vm_handler_addr;

  static thread_local std::shared_ptr<vm::ctx_t> _jmp_ctx;
  static thread_local zydis_routine_t _jmp_stream;
  static thread_local auto inst_cnt = 0ull;
  static thread_local ZydisDecodedInstruction instr;

  if (!ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(vm::util::g_decoder.get(),
                                             reinterpret_cast<void*>(address),
                                             PAGE_4KB, &instr))) {
    std::printf("> failed to decode instruction at = 0x%p\n", address);
    if ((err = uc_emu_stop(uc))) {
      std::printf("> failed to stop emulation, exiting... reason = %d\n", err);
      exit(0);
    }
    return false;
  }

  if (instr.mnemonic == ZYDIS_MNEMONIC_INVALID) {
    obj->cc_block = nullptr;
    uc_emu_stop(uc);
    return false;
  }

  // if there are over 4k instructions executed before a JMP is found then we
  // are gunna stop emulation this is a sanity check to prevent inf loops...
  if (++inst_cnt > 0x1000) {
    std::printf("> inf loop detected... stopping emulation...\n");
    obj->cc_block = nullptr, inst_cnt = 0ull;
    uc_emu_stop(uc);
    return false;
  }

  // if the native instruction is a jmp rcx/rdx... then AL will contain the vm
  // handler table index of the vm handler that the emulator is about to jmp
  // too...
  if (!(instr.mnemonic == ZYDIS_MNEMONIC_JMP &&
        instr.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
        (instr.operands[0].reg.value == ZYDIS_REGISTER_RCX ||
         instr.operands[0].reg.value == ZYDIS_REGISTER_RDX)))
    return true;

  // reset sanity check...
  inst_cnt = 0ull;

  // extract address of vm handler table...
  switch (instr.operands[0].reg.value) {
    case ZYDIS_REGISTER_RCX:
      if ((err = uc_reg_read(uc, UC_X86_REG_RCX, &vm_handler_addr))) {
        std::printf("> failed to read rcx... reason = %d\n", err);
        if ((err = uc_emu_stop(uc))) {
          std::printf("> failed to stop emulation, exiting... reason = %d\n",
                      err);
          exit(0);
        }
        return false;
      }
      break;
    case ZYDIS_REGISTER_RDX:
      if ((err = uc_reg_read(uc, UC_X86_REG_RDX, &vm_handler_addr))) {
        std::printf("> failed to read rdx... reason = %d\n", err);
        if ((err = uc_emu_stop(uc))) {
          std::printf("> failed to stop emulation, exiting... reason = %d\n",
                      err);
          exit(0);
        }
        return false;
      }
      break;
  }

  if ((err = uc_reg_read(obj->uc_ctx, UC_X86_REG_AL, &vm_handler_table_idx))) {
    std::printf("> failed to read register... reason = %d\n", err);
    if ((err = uc_emu_stop(uc))) {
      std::printf("> failed to stop emulation, exiting... reason = %d\n", err);
      exit(0);
    }
    return false;
  }

  auto& vm_handler = obj->g_vm_ctx->vm_handlers[vm_handler_table_idx];

  if ((err = obj->create_entry(&vinstr_entry))) {
    std::printf("> failed to create vinstr entry... reason = %d\n", err);
    if ((err = uc_emu_stop(uc))) {
      std::printf("> failed to stop emulation, exiting... reason = %d\n", err);
      exit(0);
    }
    return false;
  }

  // quick check to ensure sanity... things can get crazy so this is good to
  // check...
  if (vm_handler.address != vm_handler_addr ||
      vinstr_entry.vip >=
          obj->g_vm_ctx->module_base + obj->g_vm_ctx->image_size ||
      vinstr_entry.vip < obj->g_vm_ctx->module_base) {
    std::printf(
        "> vm handler index (%d) does not match vm handler address (%p)...\n",
        vm_handler_table_idx, vm_handler_addr);

    obj->cc_block = nullptr;
    if ((err = uc_emu_stop(uc))) {
      std::printf("> failed to stop emulation, exiting... reason = %d\n", err);
      exit(0);
    }

    return false;
  }

  if (!vm_handler.profile) {
    if (!g_force_emu) obj->cc_block = nullptr;

    std::printf("> please define virtual machine handler (%p): \n\n",
                (vm_handler_addr - obj->g_vm_ctx->module_base) +
                    obj->g_vm_ctx->image_base);

    vm::util::print(vm_handler.instrs);
    std::printf("\n\n");

    if (!g_force_emu) exit(0);
  }

  auto vinstr = vm::instrs::get(*obj->g_vm_ctx, vinstr_entry);

  if (!vinstr.has_value()) {
    std::printf("> failed to decode virtual instruction...\n");
    if ((err = uc_emu_stop(uc))) {
      std::printf("> failed to stop emulation, exiting... reason = %d\n", err);
      exit(0);
    }
    return false;
  }

  // log this virtual blocks vip_begin...
  if (obj->cc_block->code_block.vinstrs.empty()) {
    obj->cc_block->code_block.vip_begin =
        obj->g_vm_ctx->exec_type == vmp2::exec_type_t::forward
            ? vinstr_entry.vip - 1
            : vinstr_entry.vip + 1;

    obj->vip_begins.push_back(obj->cc_block->code_block.vip_begin);
  }

  vinstr.value().trace_data.vm_handler_rva =
      (vm_handler_addr - obj->g_vm_ctx->module_base);
  obj->cc_block->code_block.vinstrs.push_back(vinstr.value());

  if (vm_handler.profile) {
    switch (vm_handler.profile->mnemonic) {
      case vm::handler::VMEXIT: {
        obj->cc_block->code_block.jcc.has_jcc = false;
        obj->cc_block->code_block.jcc.type = vm::instrs::jcc_type::none;

        if ((err = uc_emu_stop(uc))) {
          std::printf("> failed to stop emulation, exiting... reason = %d\n",
                      err);
          exit(0);
        }
        break;
      }
      case vm::handler::JMP: {
        // get jcc data about the virtual instruction code block that was just
        // emulated...
        auto jcc_data =
            vm::instrs::get_jcc_data(*obj->g_vm_ctx, obj->cc_block->code_block);

        obj->cc_block->code_block.jcc = jcc_data.value();

        // allocate space for the cpu context and stack...
        auto new_cpu_ctx = std::make_shared<vm::emu_t::cpu_ctx_t>();

        // optimize so that we dont need to create a new vm::ctx_t every single
        // virtual JMP...
        if (obj->vm_ctxs.find(vm_handler_addr) == obj->vm_ctxs.end()) {
          obj->vm_ctxs[vm_handler_addr] = std::make_shared<vm::ctx_t>(
              obj->g_vm_ctx->module_base, obj->img_base, obj->img_size,
              vm_handler_addr - obj->g_vm_ctx->module_base);

          if (!obj->vm_ctxs[vm_handler_addr]->init()) {
            std::printf(
                "> failed to init vm::ctx_t for virtual jmp... vip = 0x%p, jmp "
                "handler = 0x%p\n",
                vinstr_entry.vip, vm_handler_addr);

            if ((err = uc_emu_stop(uc))) {
              std::printf(
                  "> failed to stop emulation, exiting... reason = %d\n", err);
              exit(0);
            }
            return false;
          }
        }

        _jmp_ctx = obj->vm_ctxs[vm_handler_addr];
        if ((err = uc_context_alloc(uc, &new_cpu_ctx->context))) {
          std::printf("> failed to allocate a unicorn context... reason = %d\n",
                      err);
          if ((err = uc_emu_stop(uc))) {
            std::printf("> failed to stop emulation, exiting... reason = %d\n",
                        err);
            exit(0);
          }
          return false;
        }

        // save the cpu's registers...
        new_cpu_ctx->rip = vm_handler_addr;
        if ((err = uc_context_save(uc, new_cpu_ctx->context))) {
          std::printf("> failed to save emulator context... reason = %d\n",
                      err);
          if ((err = uc_emu_stop(uc))) {
            std::printf("> failed to stop emulation, exiting... reason = %d\n",
                        err);
            exit(0);
          }
          return false;
        }

        // save the entire stack...
        if ((err =
                 uc_mem_read(uc, STACK_BASE, new_cpu_ctx->stack, STACK_SIZE))) {
          std::printf("> failed to read stack... reason = %d\n", err);
          if ((err = uc_emu_stop(uc))) {
            std::printf("> failed to stop emulation, exiting... reason = %d\n",
                        err);
            exit(0);
          }
          return false;
        }

        if ((err = uc_emu_stop(uc))) {
          std::printf("> failed to stop emulation, exiting... reason = %d\n",
                      err);
          exit(0);
        }

        obj->cc_block->cpu_ctx = new_cpu_ctx;
        obj->cc_block->g_vm_ctx = _jmp_ctx;
        break;
      }
      default:
        break;
    }
  }
  return true;
}

void emu_t::int_callback(uc_engine* uc, std::uint32_t intno, emu_t* obj) {
  uc_err err;
  std::uintptr_t rip = 0ull;
  static thread_local ZydisDecodedInstruction instr;

  if ((err = uc_reg_read(uc, UC_X86_REG_RIP, &rip))) {
    std::printf("> failed to read rip... reason = %d\n", err);
    return;
  }

  if (!ZYAN_SUCCESS(ZydisDecoderDecodeBuffer(vm::util::g_decoder.get(),
                                             reinterpret_cast<void*>(rip),
                                             PAGE_4KB, &instr))) {
    std::printf("> failed to decode instruction at = 0x%p\n", rip);
    if ((err = uc_emu_stop(uc))) {
      std::printf("> failed to stop emulation, exiting... reason = %d\n", err);
      exit(0);
    }
    return;
  }

  // advance rip over the instruction that caused the exception... this is
  // usually a division by 0...
  rip += instr.length;

  if ((err = uc_reg_write(uc, UC_X86_REG_RIP, &rip))) {
    std::printf("> failed to write rip... reason = %d\n", err);
    return;
  }
}

void emu_t::invalid_mem(uc_engine* uc, uc_mem_type type, uint64_t address,
                        int size, int64_t value, emu_t* obj) {
  switch (type) {
    case UC_MEM_READ_UNMAPPED: {
      uc_mem_map(uc, address & ~0xFFFull, PAGE_4KB, UC_PROT_ALL);
      std::printf(">>> reading invalid memory at address = %p, size = 0x%x\n",
                  address, size);
      break;
    }
    case UC_MEM_WRITE_UNMAPPED: {
      uc_mem_map(uc, address & ~0xFFFull, PAGE_4KB, UC_PROT_ALL);
      std::printf(
          ">>> writing invalid memory at address = %p, size = 0x%x, val = "
          "0x%x\n",
          address, size, value);
      break;
    }
    case UC_MEM_FETCH_UNMAPPED: {
      std::printf(">>> fetching invalid instructions at address = %p\n",
                  address);

      std::uintptr_t rip, rsp;
      uc_reg_read(uc, UC_X86_REG_RSP, &rsp);
      uc_mem_read(uc, rsp, &rip, sizeof rip);
      rsp += 8;
      uc_reg_write(uc, UC_X86_REG_RSP, &rsp);
      uc_reg_write(uc, UC_X86_REG_RIP, &rip);
      std::printf(">>> injecting return to try and recover... rip = %p\n", rip);
      break;
    }
    default:
      break;
  }
}
}  // namespace vm