#include <compiler.hpp>

namespace vm {
compiler_t::compiler_t(vm::ctx_t* vmctx) : vmctx(vmctx) {
  if (!parse_t::get_instance()->for_each([&](_vlabel_meta* label_data) -> bool {
        std::printf(
            "> checking label %s for invalid instructions... number of "
            "instructions = %d\n",
            label_data->label_name.c_str(), label_data->vinstrs.size());

        const auto result = std::find_if(
            label_data->vinstrs.begin(), label_data->vinstrs.end(),
            [&](const _vinstr_meta& vinstr) -> bool {
              std::printf("> vinstr name = %s, has imm = %d, imm = 0x%p\n",
                          vinstr.name.c_str(), vinstr.has_imm, vinstr.imm);

              for (auto& vm_handler : vmctx->vm_handlers)
                if (vm_handler.profile &&
                    vm_handler.profile->name == vinstr.name)
                  return false;

              std::printf(
                  "[!] this vm protected file does not have the vm handler "
                  "for: %s...\n",
                  vinstr.name.c_str());

              return true;
            });

        return result == label_data->vinstrs.end();
      })) {
    std::printf("[!] binary does not have the required vm handlers...\n");
    exit(-1);
  }

  if (!vm::handler::get_operand_transforms(vmctx->calc_jmp,
                                           calc_jmp_transforms)) {
    std::printf("[!] failed to extract calc_jmp transformations...\n");
    exit(-1);
  }

  if (!vm::instrs::get_rva_decrypt(vmctx->vm_entry, encrypt_vinstrs_rva)) {
    std::printf(
        "[!] failed to extract virtual instruction rva decryption "
        "instructions...\n");
    exit(-1);
  }

  if (!vm::transform::inverse_transforms(encrypt_vinstrs_rva)) {
    std::printf(
        "[!] failed to inverse virtual instruction rva decrypt "
        "instructions...\n");
    exit(-1);
  }
}

std::vector<vlabel_data>* compiler_t::encode() {
  parse_t::get_instance()->for_each([&](_vlabel_meta* label_data) -> bool {
    virt_labels.push_back({label_data->label_name});
    for (const auto& vinstr : label_data->vinstrs) {
      for (auto idx = 0u; idx < 256; ++idx) {
        const auto& vm_handler = vmctx->vm_handlers[idx];
        if (vm_handler.profile &&
            !vinstr.name.compare(vm_handler.profile->name)) {
          virt_labels.back().vinstrs.push_back(
              {(std::uint8_t)idx, vinstr.imm, vm_handler.profile->imm_size});
          break;
        }
      }
    }
    return true;
  });

  return &virt_labels;
}

std::vector<compiled_label_data> compiler_t::encrypt() {
  std::vector<compiled_label_data> result;
  const auto end_of_module = vmctx->image_size + vmctx->image_base;

  // decryption key starts off as the image
  // base address of the virtual instructions...
  std::uintptr_t decrypt_key = end_of_module, start_addr;
  if (vmctx->exec_type == vmp2::exec_type_t::backward)
    std::for_each(
        virt_labels.begin()->vinstrs.begin(),
        virt_labels.begin()->vinstrs.end(), [&](const vinstr_data& vinstr) {
          (++decrypt_key) += vinstr.imm_size ? vinstr.imm_size / 8 : 0;
        });

  const auto opcode_fetch = std::find_if(
      vmctx->calc_jmp.begin(), vmctx->calc_jmp.end(),
      [](const zydis_instr_t& instr_data) -> bool {
        // mov/movsx/movzx rax/eax/ax/al, [rsi]
        return instr_data.instr.operand_count > 1 &&
               (instr_data.instr.mnemonic == ZYDIS_MNEMONIC_MOV ||
                instr_data.instr.mnemonic == ZYDIS_MNEMONIC_MOVSX ||
                instr_data.instr.mnemonic == ZYDIS_MNEMONIC_MOVZX) &&
               instr_data.instr.operands[0].type ==
                   ZYDIS_OPERAND_TYPE_REGISTER &&
               util::reg::to64(instr_data.instr.operands[0].reg.value) ==
                   ZYDIS_REGISTER_RAX &&
               instr_data.instr.operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
               instr_data.instr.operands[1].mem.base == ZYDIS_REGISTER_RSI;
      });

  if (opcode_fetch == vmctx->calc_jmp.end()) {
    std::printf(
        "> critical error trying to find opcode fetch inside of "
        "compiler_t::encrypt...\n");
    exit(0);
  }

  start_addr = decrypt_key - 1;  // make it zero based...
  std::for_each(
      virt_labels.begin(), virt_labels.end(), [&](vm::vlabel_data& label) {
        // sometimes there is a mov al, [rsi-1]... we want that disp...
        if (opcode_fetch->instr.operands[1].mem.disp.has_displacement)
          start_addr +=
              std::abs(opcode_fetch->instr.operands[1].mem.disp.value);

        decrypt_key = start_addr;
        result.push_back({label.label_name, start_addr});

        if (vmctx->exec_type == vmp2::exec_type_t::forward) {
          std::for_each(
              label.vinstrs.begin(), label.vinstrs.end(),
              [&](vm::vinstr_data& vinstr) {
                std::uint8_t opcode = vinstr.vm_handler;
                std::uint64_t operand = 0u;

                // encrypt opcode...
                std::tie(opcode, decrypt_key) = vm::instrs::encrypt_operand(
                    calc_jmp_transforms, vinstr.vm_handler, decrypt_key);

                // if there is an operand then we will encrypt that as well..
                if (vmctx->vm_handlers[vinstr.vm_handler].imm_size) {
                  auto& vm_handler_transforms =
                      vmctx->vm_handlers[vinstr.vm_handler].transforms;
                  std::tie(operand, decrypt_key) = vm::instrs::encrypt_operand(
                      vm_handler_transforms, vinstr.operand, decrypt_key);
                } else  // else just push back the opcode...
                {
                  result.back().vinstrs.push_back(opcode);
                  return;  // finished here...
                }

                result.back().vinstrs.push_back(opcode);
                for (auto idx = 0u;
                     idx < vmctx->vm_handlers[vinstr.vm_handler].imm_size / 8;
                     ++idx)
                  result.back().vinstrs.push_back(
                      reinterpret_cast<std::uint8_t*>(&vinstr.operand)[idx]);
              });
        } else {
          std::for_each(
              label.vinstrs.begin(), label.vinstrs.end(),
              [&](vm::vinstr_data& vinstr) {
                std::uint8_t opcode = vinstr.vm_handler;
                std::uint64_t operand = 0u;

                // encrypt opcode...
                std::tie(opcode, decrypt_key) = vm::instrs::encrypt_operand(
                    calc_jmp_transforms, vinstr.vm_handler, decrypt_key);

                // if there is an operand then we will encrypt that as well..
                if (vmctx->vm_handlers[vinstr.vm_handler].imm_size) {
                  auto& vm_handler_transforms =
                      vmctx->vm_handlers[vinstr.vm_handler].transforms;
                  std::tie(operand, decrypt_key) = vm::instrs::encrypt_operand(
                      vm_handler_transforms, vinstr.operand, decrypt_key);
                } else  // else just push back the opcode...
                {
                  result.back().vinstrs.insert(result.back().vinstrs.begin(), 1,
                                               opcode);
                  return;  // finished here...
                }

                // operand goes first, then opcode when vip advances
                // backwards...
                std::vector<std::uint8_t> _temp;
                for (auto idx = 0u;
                     idx < vmctx->vm_handlers[vinstr.vm_handler].imm_size / 8;
                     ++idx)
                  _temp.push_back(
                      reinterpret_cast<std::uint8_t*>(&operand)[idx]);

                result.back().vinstrs.insert(result.back().vinstrs.begin(),
                                             _temp.begin(), _temp.end());
                result.back().vinstrs.insert(
                    result.back().vinstrs.begin() + _temp.size(), opcode);
              });
        }

        result.back().enc_alloc_rva = encrypt_rva(start_addr);
        start_addr +=
            result.back().vinstrs.size() - 1;  // make it zero based...
      });

  return result;
}

std::uint64_t compiler_t::encrypt_rva(std::uint64_t rva) {
  for (auto& instr : encrypt_vinstrs_rva)
    rva = vm::transform::apply(
        instr.operands[0].size, instr.mnemonic, rva,
        transform::has_imm(&instr) ? instr.operands[1].imm.value.u : 0);

  return rva;
}
}  // namespace vm