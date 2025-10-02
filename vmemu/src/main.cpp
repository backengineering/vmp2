#include <cli-parser.hpp>
#include <fstream>
#include <iostream>
#include <thread>

#include "unpacker.hpp"
#include "vmemu_t.hpp"

#define NUM_THREADS 20

int __cdecl main(int argc, const char* argv[]) {
  argparse::argument_parser_t parser("VMEmu",
                                     "VMProtect 2 VM Handler Emulator");
  parser.add_argument()
      .name("--vmentry")
      .description("relative virtual address to a vm entry...");
  parser.add_argument()
      .name("--bin")
      .description("path to unpacked virtualized binary...")
      .required(true);
  parser.add_argument()
      .name("--out")
      .description("output file name...")
      .required(true);
  parser.add_argument().name("--unpack").description("unpack a vmp2 binary...");
  parser.add_argument()
      .names({"-f", "--force"})
      .description("force emulation of unknown vm handlers...");
  parser.add_argument()
      .name("--emuall")
      .description(
          "scan for all vm enters and trace all of them... this may take a few "
          "minutes...");

  parser.enable_help();
  auto result = parser.parse(argc, argv);

  if (result) {
    std::printf("[!] error parsing commandline arguments... reason = %s\n",
                result.what().c_str());
    return -1;
  }

  if (parser.exists("help")) {
    parser.print_help();
    return 0;
  }

  vm::util::init();
  vm::g_force_emu = parser.exists("force");

  std::vector<std::uint8_t> module_data, tmp, unpacked_bin;
  if (!vm::util::open_binary_file(parser.get<std::string>("bin"),
                                  module_data)) {
    std::printf("[!] failed to open binary file...\n");
    return -1;
  }

  // only support 64bit executables...
  auto img = reinterpret_cast<win::image_t<>*>(module_data.data());
  if (img->get_nt_headers()->file_header.machine != win::machine_id::amd64) {
    std::printf("[!] only 64bit executables are supported...\n");
    return -1;
  }

  auto image_size = img->get_nt_headers()->optional_header.size_image;
  const auto image_base = img->get_nt_headers()->optional_header.image_base;

  // page align the vector allocation so that unicorn-engine is happy girl...
  tmp.resize(image_size + PAGE_4KB);
  const std::uintptr_t module_base =
      reinterpret_cast<std::uintptr_t>(tmp.data()) +
      (PAGE_4KB - (reinterpret_cast<std::uintptr_t>(tmp.data()) & 0xFFFull));

  std::memcpy((void*)module_base, module_data.data(), 0x1000);
  std::for_each(img->get_nt_headers()->get_sections(),
                img->get_nt_headers()->get_sections() +
                    img->get_nt_headers()->file_header.num_sections,
                [&](const auto& section_header) {
                  std::memcpy(
                      (void*)(module_base + section_header.virtual_address),
                      module_data.data() + section_header.ptr_raw_data,
                      section_header.size_raw_data);
                });

  auto win_img = reinterpret_cast<win::image_t<>*>(module_base);

  auto basereloc_dir =
      win_img->get_directory(win::directory_id::directory_entry_basereloc);

  auto reloc_dir = reinterpret_cast<win::reloc_directory_t*>(
      basereloc_dir->rva + module_base);

  win::reloc_block_t* reloc_block = &reloc_dir->first_block;

  // apply relocations to all sections...
  while (reloc_block->base_rva && reloc_block->size_block) {
    std::for_each(reloc_block->begin(), reloc_block->end(),
                  [&](win::reloc_entry_t& entry) {
                    switch (entry.type) {
                      case win::reloc_type_id::rel_based_dir64: {
                        auto reloc_at = reinterpret_cast<std::uintptr_t*>(
                            entry.offset + reloc_block->base_rva + module_base);
                        *reloc_at = module_base + ((*reloc_at) - image_base);
                        break;
                      }
                      default:
                        break;
                    }
                  });

    reloc_block = reloc_block->next();
  }

  std::printf("> image base = %p, image size = %p, module base = %p\n",
              image_base, image_size, module_base);

  if (!image_base || !image_size || !module_base) {
    std::printf("[!] failed to open binary on disk...\n");
    return -1;
  }

  if (parser.exists("vmentry")) {
    const auto vm_entry_rva =
        std::strtoull(parser.get<std::string>("vmentry").c_str(), nullptr, 16);

    std::vector<vm::instrs::code_block_t> code_blocks;
    vm::ctx_t vmctx(module_base, image_base, image_size, vm_entry_rva);

    if (!vmctx.init()) {
      std::printf(
          "[!] failed to init vmctx... this can be for many reasons..."
          " try validating your vm entry rva... make sure the binary is "
          "unpacked and is"
          "protected with VMProtect 2...\n");
      return -1;
    }

    vm::emu_t emu(&vmctx);

    if (!emu.init()) {
      std::printf("[!] failed to init emulator...\n");
      return -1;
    }

    if (!emu.get_trace(code_blocks)) {
      std::printf(
          "[!] something failed during tracing, review the console for more "
          "information...\n");
      return -1;
    }

    std::printf("> number of blocks = %d\n", code_blocks.size());
    for (auto& code_block : code_blocks) {
      std::printf("> code block starts at = %p\n", code_block.vip_begin);
      std::printf("> number of virtual instructions = %d\n",
                  code_block.vinstrs.size());
      std::printf("> does this code block have a jcc? %s\n",
                  code_block.jcc.has_jcc ? "yes" : "no");

      if (code_block.jcc.has_jcc) {
        switch (code_block.jcc.type) {
          case vm::instrs::jcc_type::branching: {
            std::printf("> branch 1 = %p, branch 2 = %p\n",
                        code_block.jcc.block_addr[0],
                        code_block.jcc.block_addr[1]);
            break;
          }
          case vm::instrs::jcc_type::absolute: {
            std::printf("> branch 1 = %p\n", code_block.jcc.block_addr[0]);
            break;
          }
          case vm::instrs::jcc_type::switch_case: {
            std::printf("> switch case blocks:\n");
            for (auto idx = 0u; idx < code_block.jcc.block_addr.size(); ++idx)
              std::printf("    case block at = 0x%p\n",
                          code_block.jcc.block_addr[idx]);
            break;
          }
        }
      }
    }

    std::printf("> serializing results....\n");
    vmp2::v4::file_header file_header;
    file_header.magic = VMP_MAGIC;
    file_header.epoch_time = std::time(nullptr);
    file_header.version = vmp2::version_t::v4;
    file_header.module_base = module_base;
    file_header.image_base = image_base;
    file_header.vm_entry_rva = vm_entry_rva;
    file_header.module_offset = sizeof file_header;
    file_header.module_size = image_size;
    file_header.rtn_count = 1;
    file_header.rtn_offset = image_size + sizeof file_header;

    vmp2::v4::rtn_t rtn;
    std::ofstream output(parser.get<std::string>("out"), std::ios::binary);
    output.write(reinterpret_cast<const char*>(&file_header),
                 sizeof file_header);
    output.write(reinterpret_cast<const char*>(module_base), image_size);

    std::vector<vmp2::v4::code_block_t*> vmp2_blocks;
    for (const auto& code_block : code_blocks) {
      const auto _code_block_size =
          sizeof(vmp2::v4::code_block_t) +
          (code_block.jcc.block_addr.size() * 8) +
          code_block.vinstrs.size() * sizeof(vm::instrs::virt_instr_t);

      vmp2::v4::code_block_t* _code_block =
          reinterpret_cast<vmp2::v4::code_block_t*>(malloc(_code_block_size));

      // serialize block meta data...
      _code_block->vip_begin = code_block.vip_begin;
      _code_block->next_block_offset = _code_block_size;
      _code_block->vinstr_count = code_block.vinstrs.size();
      _code_block->has_jcc = code_block.jcc.has_jcc;
      _code_block->jcc_type = code_block.jcc.type;
      _code_block->num_block_addrs = code_block.jcc.block_addr.size();

      // serialize jcc branches...
      for (auto idx = 0u; idx < code_block.jcc.block_addr.size(); ++idx)
        _code_block->branch_addr[idx] = code_block.jcc.block_addr[idx];

      auto block_vinstrs = reinterpret_cast<vm::instrs::virt_instr_t*>(
          reinterpret_cast<std::uintptr_t>(_code_block) +
          sizeof(vmp2::v4::code_block_t) +
          (code_block.jcc.block_addr.size() * 8));

      for (auto idx = 0u; idx < code_block.vinstrs.size(); ++idx)
        block_vinstrs[idx] = code_block.vinstrs[idx];

      vmp2_blocks.push_back(_code_block);
    }

    std::size_t code_blocks_size = sizeof(vmp2::v4::rtn_t::size) +
                                   sizeof(vmp2::v4::rtn_t::code_block_count) +
                                   sizeof(vmp2::v4::rtn_t::vm_enter_offset);

    std::for_each(vmp2_blocks.begin(), vmp2_blocks.end(),
                  [&](vmp2::v4::code_block_t* vmp2_block) -> void {
                    code_blocks_size += vmp2_block->next_block_offset;
                  });

    rtn.size = code_blocks_size;
    rtn.code_block_count = vmp2_blocks.size();
    rtn.vm_enter_offset = vm_entry_rva;

    output.write(reinterpret_cast<const char*>(&rtn),
                 sizeof(vmp2::v4::rtn_t::size) +
                     sizeof(vmp2::v4::rtn_t::code_block_count) +
                     sizeof(vmp2::v4::rtn_t::vm_enter_offset));

    std::for_each(vmp2_blocks.begin(), vmp2_blocks.end(),
                  [&](vmp2::v4::code_block_t* vmp2_block) -> void {
                    output.write(reinterpret_cast<const char*>(vmp2_block),
                                 vmp2_block->next_block_offset);
                    free(vmp2_block);
                  });
    output.close();
  } else if (parser.exists("unpack")) {
    engine::unpack_t unpacker(parser.get<std::string>("bin"), module_data);

    if (!unpacker.init()) {
      std::printf("> failed to init unpacker...\n");
      return -1;
    }

    if (!unpacker.unpack(unpacked_bin)) {
      std::printf("> failed to unpack binary... refer to log above...\n");
      return -1;
    }

    std::printf("> writing result to = %s\n",
                parser.get<std::string>("out").c_str());

    std::ofstream output(parser.get<std::string>("out"), std::ios::binary);
    output.write(reinterpret_cast<char*>(unpacked_bin.data()),
                 unpacked_bin.size());

    output.close();
  } else if (parser.exists("emuall")) {
    auto entries = vm::locate::get_vm_entries(module_base, image_size);

    std::vector<
        std::pair<std::uintptr_t, std::vector<vm::instrs::code_block_t> > >
        virt_rtns;

    std::vector<std::thread> threads;
    for (const auto& [vm_enter_offset, encrypted_rva, hndlr_tble] : entries) {
      if (threads.size() == NUM_THREADS) {
        std::for_each(threads.begin(), threads.end(),
                      [&](std::thread& t) { t.join(); });
        threads.clear();
      }

      threads.emplace_back(std::thread([vm_enter_offset = vm_enter_offset,
                                        module_base = module_base,
                                        image_base = image_base,
                                        image_size = image_size,
                                        &virt_rtns = virt_rtns]() {
        std::printf("> emulating vm enter at rva = 0x%x\n", vm_enter_offset);
        vm::ctx_t vm_ctx(module_base, image_base, image_size, vm_enter_offset);

        if (!vm_ctx.init()) {
          std::printf(
              "[!] failed to init vmctx... this can be for many reasons..."
              " try validating your vm entry rva... make sure the binary is "
              "unpacked and is"
              "protected with VMProtect 2...\n");
          return;
        }

        vm::emu_t emu(&vm_ctx);

        if (!emu.init()) {
          std::printf("[!] failed to init emulator...\n");
          return;
        }

        std::vector<vm::instrs::code_block_t> code_blocks;

        if (!emu.get_trace(code_blocks)) {
          std::printf(
              "[!] something failed during tracing, review the console for "
              "more "
              "information...\n");
          return;
        }

        std::printf("> number of blocks = %d\n", code_blocks.size());
        virt_rtns.push_back({vm_enter_offset, code_blocks});
      }));
    }

    std::for_each(threads.begin(), threads.end(),
                  [&](std::thread& t) { t.join(); });

    std::printf("> traced %d virtual routines...\n", virt_rtns.size());
    std::printf("> serializing results....\n");

    vmp2::v4::file_header file_header;
    file_header.magic = VMP_MAGIC;
    file_header.epoch_time = std::time(nullptr);
    file_header.version = vmp2::version_t::v4;
    file_header.module_base = module_base;
    file_header.image_base = image_base;
    file_header.vm_entry_rva = 0ull;
    file_header.module_offset = sizeof file_header;
    file_header.module_size = image_size;
    file_header.rtn_count = virt_rtns.size();
    file_header.rtn_offset = image_size + sizeof file_header;

    std::ofstream output(parser.get<std::string>("out"), std::ios::binary);
    output.write(reinterpret_cast<const char*>(&file_header),
                 sizeof file_header);
    output.write(reinterpret_cast<const char*>(module_base), image_size);

    for (auto& [vm_enter_offset, virt_rtn] : virt_rtns) {
      vmp2::v4::rtn_t rtn{(u32)virt_rtn.size()};
      std::vector<vmp2::v4::code_block_t*> vmp2_blocks;

      for (const auto& code_block : virt_rtn) {
        const auto _code_block_size =
            sizeof(vmp2::v4::code_block_t) +
            (code_block.jcc.block_addr.size() * 8) +
            code_block.vinstrs.size() * sizeof(vm::instrs::virt_instr_t);

        vmp2::v4::code_block_t* _code_block =
            reinterpret_cast<vmp2::v4::code_block_t*>(malloc(_code_block_size));

        // serialize block meta data...
        _code_block->vip_begin = code_block.vip_begin;
        _code_block->next_block_offset = _code_block_size;
        _code_block->vinstr_count = code_block.vinstrs.size();
        _code_block->has_jcc = code_block.jcc.has_jcc;
        _code_block->jcc_type = code_block.jcc.type;
        _code_block->num_block_addrs = code_block.jcc.block_addr.size();

        // serialize jcc branches...
        for (auto idx = 0u; idx < code_block.jcc.block_addr.size(); ++idx)
          _code_block->branch_addr[idx] = code_block.jcc.block_addr[idx];

        auto block_vinstrs = reinterpret_cast<vm::instrs::virt_instr_t*>(
            reinterpret_cast<std::uintptr_t>(_code_block) +
            sizeof(vmp2::v4::code_block_t) +
            (code_block.jcc.block_addr.size() * 8));

        for (auto idx = 0u; idx < code_block.vinstrs.size(); ++idx)
          block_vinstrs[idx] = code_block.vinstrs[idx];

        vmp2_blocks.push_back(_code_block);
      }

      std::size_t code_blocks_size = sizeof(vmp2::v4::rtn_t::size) +
                                     sizeof(vmp2::v4::rtn_t::vm_enter_offset) +
                                     sizeof(vmp2::v4::rtn_t::code_block_count);

      std::for_each(vmp2_blocks.begin(), vmp2_blocks.end(),
                    [&](vmp2::v4::code_block_t* vmp2_block) -> void {
                      code_blocks_size += vmp2_block->next_block_offset;
                    });

      rtn.size = code_blocks_size;
      rtn.code_block_count = vmp2_blocks.size();
      rtn.vm_enter_offset = vm_enter_offset;

      output.write(reinterpret_cast<const char*>(&rtn),
                   sizeof(vmp2::v4::rtn_t::size) +
                       sizeof(vmp2::v4::rtn_t::code_block_count) +
                       sizeof(vmp2::v4::rtn_t::vm_enter_offset));

      std::for_each(vmp2_blocks.begin(), vmp2_blocks.end(),
                    [&](vmp2::v4::code_block_t* vmp2_block) -> void {
                      output.write(reinterpret_cast<const char*>(vmp2_block),
                                   vmp2_block->next_block_offset);
                      free(vmp2_block);
                    });
    }
    output.close();
  }
}