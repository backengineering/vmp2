#include <ZydisExportConfig.h>

#include <algorithm>
#include <argparse.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nt/image.hpp>
#include <vmprofiler.hpp>

auto open_binary_file(const std::string &file, std::vector<uint8_t> &data)
    -> bool {
  std::ifstream fstr(file, std::ios::binary);

  if (!fstr.is_open()) return false;

  fstr.unsetf(std::ios::skipws);
  fstr.seekg(0, std::ios::end);

  const auto file_size = fstr.tellg();

  fstr.seekg(NULL, std::ios::beg);
  data.reserve(static_cast<uint32_t>(file_size));
  data.insert(data.begin(), std::istream_iterator<uint8_t>(fstr),
              std::istream_iterator<uint8_t>());
  return true;
}

int __cdecl main(int argc, const char *argv[]) {
  argparse::ArgumentParser parser("vmprofiler-cli",
                                     "virtual machine information inspector");
  parser.add_argument()
      .names({"--bin", "--vmpbin"})
      .description("unpacked binary protected with VMProtect 2");
  parser.add_argument()
      .names({"--vmentry", "--entry"})
      .description("rva to push prior to a vm_entry");
  parser.add_argument()
      .name("--showhandlers")
      .description("show all vm handlers...");
  parser.add_argument()
      .name("--showhandler")
      .description("show a specific vm handler given its index...");
  parser.add_argument()
      .name("--indexes")
      .description(
          "displays vm handler table indexes for a given vm handler name such "
          "as 'READQ', or 'WRITEQ'...");
  parser.add_argument()
      .name("--findentries")
      .description(
          "finds all virtual machine entries and displays information for "
          "them...");

  parser.enable_help();
  auto err = parser.parse(argc, argv);

  if (err) {
    std::cout << err << std::endl;
    return -1;
  }

  if (parser.exists("help")) {
    parser.print_help();
    return 0;
  }

  if (parser.exists("bin")) {
    if (!std::filesystem::exists(parser.get<std::string>("bin"))) {
      std::printf(
          "> path to protected file is invalid... check your cli args...\n");
      return -1;
    }

    std::vector<std::uint8_t> module_data, tmp;
    if (!open_binary_file(parser.get<std::string>("bin"), module_data)) {
      std::printf("[!] failed to open binary = %s\n",
                  parser.get<std::string>("bin").c_str());
      return -1;
    }

    auto img = reinterpret_cast<win::image_t<> *>(module_data.data());
    auto image_size = img->get_nt_headers()->optional_header.size_image;

    tmp.resize(image_size);
    std::memcpy(tmp.data(), module_data.data(), 0x1000);
    std::for_each(img->get_nt_headers()->get_sections(),
                  img->get_nt_headers()->get_sections() +
                      img->get_nt_headers()->file_header.num_sections,
                  [&](const auto &section_header) {
                    std::memcpy(
                        tmp.data() + section_header.virtual_address,
                        module_data.data() + section_header.ptr_raw_data,
                        section_header.size_raw_data);
                  });

    vm::util::init();
    const auto module_base = reinterpret_cast<std::uintptr_t>(tmp.data());
    const auto image_base = img->get_nt_headers()->optional_header.image_base;

    if (parser.exists("vmentry")) {
      const auto vm_entry_rva = std::strtoull(
          parser.get<std::string>("vmentry").c_str(), nullptr, 16);

      std::printf(
          "> module base = %p, image base = %p, image size = %p, vm entry = "
          "0x%x\n",
          module_base, image_base, image_size, vm_entry_rva);

      vm::ctx_t vmctx(module_base, image_base, image_size, vm_entry_rva);

      if (!vmctx.init()) {
        std::printf(
            "[!] failed to init vm::ctx_t... make sure all cli arguments are "
            "correct!\n");
        return -1;
      }

      std::puts(
          "======================== [vm entry] ========================\n");

      std::for_each(vmctx.vm_entry.begin(), vmctx.vm_entry.end(),
                    [&](zydis_instr_t &instr) {
                      instr.addr -= module_base;
                      instr.addr += image_base;
                    });

      vm::util::print(vmctx.vm_entry);
      std::puts(
          "======================== [calc jmp] ========================\n");

      std::for_each(vmctx.calc_jmp.begin(), vmctx.calc_jmp.end(),
                    [&](zydis_instr_t &instr) {
                      instr.addr -= module_base;
                      instr.addr += image_base;
                    });
      std::puts(
          "============================================================\n");
      std::printf("> vip advancement = %s\n\n",
                  vmctx.exec_type == vmp2::exec_type_t::forward ? "forward"
                                                                : "backward");

      if (parser.exists("showhandlers")) {
        for (auto idx = 0u; idx < vmctx.vm_handlers.size(); ++idx) {
          std::printf(
              "======================== [%s #%d] ========================\n",
              vmctx.vm_handlers[idx].profile
                  ? vmctx.vm_handlers[idx].profile->name
                  : "UNK",
              idx);

          vm::util::print(vmctx.vm_handlers[idx].instrs);

          // if there is no imm then there are no transforms...
          if (!vmctx.vm_handlers[idx].imm_size) {
            std::puts("\n");
            continue;
          }

          std::puts(
              "======================== [transforms] "
              "========================\n");
          for (auto &[mnemonic, instr] : vmctx.vm_handlers[idx].transforms) {
            if (instr.mnemonic == ZYDIS_MNEMONIC_INVALID) continue;

            vm::util::print(instr);
          }
          std::puts("\n");
        }
      }
      if (parser.exists("showhandler")) {
        const auto vm_handler_idx = std::strtoul(
            parser.get<std::string>("showhandler").c_str(), nullptr, 10);

        if (vm_handler_idx > 256) {
          std::printf("> invalid vm handler index... too large...\n");
          return -1;
        }

        std::printf(
            "======================== [%s #%d] ========================\n",
            vmctx.vm_handlers[vm_handler_idx].profile
                ? vmctx.vm_handlers[vm_handler_idx].profile->name
                : "UNK",
            vm_handler_idx);

        vm::util::print(vmctx.vm_handlers[vm_handler_idx].instrs);

        // if there is no imm then there are no transforms...
        if (!vmctx.vm_handlers[vm_handler_idx].imm_size) {
          std::puts("\n");
          return {};
        }

        std::puts(
            "======================== [transforms] ========================\n");

        for (auto &[mnemonic, instr] :
             vmctx.vm_handlers[vm_handler_idx].transforms) {
          if (instr.mnemonic == ZYDIS_MNEMONIC_INVALID) continue;

          vm::util::print(instr);
        }
        std::puts("\n");
      }

      if (parser.exists("indexes")) {
        const auto handler_name = parser.get<std::string>("indexes");
        std::printf("{\n");
        for (auto idx = 0u; idx < vmctx.vm_handlers.size(); ++idx)
          if (vmctx.vm_handlers[idx].profile &&
              !strcmp(vmctx.vm_handlers[idx].profile->name,
                      handler_name.c_str()))
            std::printf("\t0x%x,\n", idx);
        std::printf("}\n");
      }
    }
    if (parser.exists("findentries")) {
      std::printf("> scanning for all entries...\n");
      const auto entries = vm::locate::get_vm_entries(module_base, image_size);
      std::printf("> number of entries located = %d\n", entries.size());

      for (const auto &vm_enter : entries) {
        vm::ctx_t vmctx(module_base, image_base, image_size, vm_enter.rva);

        if (!vmctx.init()) {
          std::printf(
              "[!] failed to init vm::ctx_t... make sure all cli arguments are "
              "correct!\n");
          return -1;
        }

        std::puts(
            "======================== [vm entry] ========================\n");

        std::for_each(vmctx.vm_entry.begin(), vmctx.vm_entry.end(),
                      [&](zydis_instr_t &instr) {
                        instr.addr -= module_base;
                        instr.addr += image_base;
                      });

        vm::util::print(vmctx.vm_entry);
        std::puts(
            "======================== [calc jmp] ========================\n");

        std::for_each(vmctx.calc_jmp.begin(), vmctx.calc_jmp.end(),
                      [&](zydis_instr_t &instr) {
                        instr.addr -= module_base;
                        instr.addr += image_base;
                      });

        vm::util::print(vmctx.calc_jmp);
        std::puts(
            "============================================================\n");

        std::printf("> vip advancement = %s\n\n",
                    vmctx.exec_type == vmp2::exec_type_t::forward ? "forward"
                                                                  : "backward");

        if (parser.exists("showhandlers")) {
          for (auto idx = 0u; idx < vmctx.vm_handlers.size(); ++idx) {
            std::printf(
                "======================== [%s #%d] ========================\n",
                vmctx.vm_handlers[idx].profile
                    ? vmctx.vm_handlers[idx].profile->name
                    : "UNK",
                idx);

            vm::util::print(vmctx.vm_handlers[idx].instrs);

            // if there is no imm then there are no transforms...
            if (!vmctx.vm_handlers[idx].imm_size) {
              std::puts("\n");
              continue;
            }

            std::puts(
                "======================== [transforms] "
                "========================\n");
            for (auto &[mnemonic, instr] : vmctx.vm_handlers[idx].transforms) {
              if (instr.mnemonic == ZYDIS_MNEMONIC_INVALID) continue;

              vm::util::print(instr);
            }
            std::puts("\n");
          }
        }

        if (parser.exists("indexes")) {
          const auto handler_name = parser.get<std::string>("indexes");
          std::printf("{\n");
          for (auto idx = 0u; idx < vmctx.vm_handlers.size(); ++idx)
            if (vmctx.vm_handlers[idx].profile &&
                !strcmp(vmctx.vm_handlers[idx].profile->name,
                        handler_name.c_str()))
              std::printf("\t0x%x,\n", idx);
          std::printf("}\n");
        }
      }
    }
  }
}