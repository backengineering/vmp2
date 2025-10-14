#include <unpacker.hpp>

namespace engine {
unpack_t::unpack_t(const std::string& module_name,
                   const std::vector<std::uint8_t>& packed_bin)
    : module_name(module_name),
      bin(packed_bin),
      uc_ctx(nullptr),
      heap_offset(0ull),
      pack_section_offset(0ull) {
  win_img = reinterpret_cast<win::image_t<>*>(bin.data());
  img_base = win_img->get_nt_headers()->optional_header.image_base;
  img_size = win_img->get_nt_headers()->optional_header.size_image;
  std::printf("> image base = 0x%p, image size = 0x%x\n", img_base, img_size);
}

unpack_t::~unpack_t(void) {
  if (uc_ctx)
    uc_close(uc_ctx);

  for (auto& ptr : uc_hooks)
    if (ptr)
      delete ptr;
}

bool unpack_t::init(void) {
  uc_err err;
  if ((err = uc_open(UC_ARCH_X86, UC_MODE_64, &uc_ctx))) {
    std::printf("> uc_open err = %d\n", err);
    return false;
  }

  if ((err = uc_mem_map(uc_ctx, IAT_VECTOR_TABLE, PAGE_4KB, UC_PROT_ALL))) {
    std::printf("> uc_mem_map iat vector table err = %d\n", err);
    return false;
  }

  if ((err = uc_mem_map(uc_ctx, STACK_BASE, STACK_SIZE, UC_PROT_ALL))) {
    std::printf("> uc_mem_map stack err, reason = %d\n", err);
    return false;
  }

  if ((err = uc_mem_map(uc_ctx, img_base, img_size, UC_PROT_ALL))) {
    std::printf("> map memory failed, reason = %d\n", err);
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

  map_bin.resize(img_size);
  memcpy(map_bin.data(),
         bin.data(),  // copies pe headers (includes section headers)
         win_img->get_nt_headers()->optional_header.size_headers);

  win::section_header_t *sec_begin = win_img->get_nt_headers()->get_sections(),
                        *sec_end =
                            sec_begin +
                            win_img->get_nt_headers()->file_header.num_sections;

  std::for_each(
      sec_begin, sec_end, [&](const win::section_header_t& sec_header) {
        memcpy(map_bin.data() + sec_header.virtual_address,
               bin.data() + sec_header.ptr_raw_data, sec_header.size_raw_data);
      });

  // install iat hooks...
  for (auto import_dir = reinterpret_cast<win::import_directory_t*>(
           win_img->get_directory(win::directory_id::directory_entry_import)
               ->rva +
           map_bin.data());
       import_dir->rva_name; ++import_dir) {
    for (auto iat_thunk = reinterpret_cast<win::image_thunk_data_t<>*>(
             import_dir->rva_first_thunk + map_bin.data());
         iat_thunk->address; ++iat_thunk) {
      if (iat_thunk->is_ordinal)
        continue;

      auto iat_name = reinterpret_cast<win::image_named_import_t*>(
          iat_thunk->address + map_bin.data());

      if (iat_hooks.find(iat_name->name) != iat_hooks.end()) {
        std::printf("> iat hooking %s to vector table %p\n", iat_name->name,
                    iat_hooks[iat_name->name].first + IAT_VECTOR_TABLE);

        iat_thunk->function =
            iat_hooks[iat_name->name].first + IAT_VECTOR_TABLE;
      }
    }
  }

  // map the entire map buffer into unicorn-engine since we have set everything
  // else up...
  if ((err = uc_mem_write(uc_ctx, img_base, map_bin.data(), map_bin.size()))) {
    std::printf("> failed to write memory... reason = %d\n", err);
    return false;
  }

  // setup unicorn-engine hooks on IAT vector table, sections with 0 raw
  // size/ptr, and an invalid memory handler...

  uc_hooks.push_back(new uc_hook);
  if ((err = uc_hook_add(uc_ctx, uc_hooks.back(), UC_HOOK_CODE,
                         (void*)&engine::unpack_t::iat_dispatcher, this,
                         IAT_VECTOR_TABLE, IAT_VECTOR_TABLE + PAGE_4KB))) {
    std::printf("> uc_hook_add error, reason = %d\n", err);
    return false;
  }

  uc_hooks.push_back(new uc_hook);
  if ((err = uc_hook_add(
           uc_ctx, uc_hooks.back(),
           UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_WRITE_UNMAPPED |
               UC_HOOK_MEM_FETCH_UNMAPPED | UC_HOOK_INSN_INVALID,
           (void*)&engine::unpack_t::invalid_mem, this, true, false))) {
    std::printf("> uc_hook_add error, reason = %d\n", err);
    return false;
  }

  return true;
}

bool unpack_t::unpack(std::vector<std::uint8_t>& output) {
  uc_err err;
  auto nt_headers = win_img->get_nt_headers();
  std::uintptr_t rip = nt_headers->optional_header.entry_point + img_base,
                 rsp = (STACK_BASE + STACK_SIZE) - PAGE_4KB;

  if ((err = uc_reg_write(uc_ctx, UC_X86_REG_RSP, &rsp))) {
    std::printf("> uc_reg_write error, reason = %d\n", err);
    return false;
  }

  if ((err = uc_reg_write(uc_ctx, UC_X86_REG_RIP, &rip))) {
    std::printf("> uc_reg_write error, reason = %d\n", err);
    return false;
  }

  std::printf("> beginning execution at = %p\n", rip);
  uc_emu_start(uc_ctx, rip, 0ull, 0ull, 0ull);

  output.resize(img_size);
  if ((err = uc_mem_read(uc_ctx, img_base, output.data(), output.size()))) {
    std::printf("> uc_mem_read failed... err = %d\n", err);
    return false;
  }

  auto output_img = reinterpret_cast<win::image_t<>*>(output.data());
  auto sections = output_img->get_nt_headers()->get_sections();
  auto section_cnt = output_img->get_file_header()->num_sections;

  // { section virtual address -> vector of section offset to reloc }
  std::map<std::uint32_t, std::vector<std::uint16_t>> new_relocs;

  // search executable sections for MOV RAX, 00 00 00 00 00 00 00 00...
  std::for_each(
      sections, sections + section_cnt, [&](win::section_header_t& header) {
        if (header.characteristics.mem_execute) {
          auto result = output.data() + header.virtual_address;

          do {
            result = reinterpret_cast<std::uint8_t*>(vm::locate::sigscan(
                result,
                header.virtual_size -
                    (reinterpret_cast<std::uintptr_t>(result) -
                     (header.virtual_address +
                      reinterpret_cast<std::uintptr_t>(output.data()))),
                MOV_RAX_0_SIG, MOV_RAX_0_MASK));

            if (result) {
              result += 2;  // advance ahead of the 0x48 0xB8...

              // offset from section begin...
              auto reloc_offset = (reinterpret_cast<std::uintptr_t>(result)) -
                                  reinterpret_cast<std::uintptr_t>(
                                      output.data() + header.virtual_address);

              new_relocs[(header.virtual_address + reloc_offset) & ~0xFFFull]
                  .push_back(reloc_offset);
            }

          } while (result);
        }

        header.ptr_raw_data = header.virtual_address;
        header.size_raw_data = header.virtual_size;
      });

  // determines if a relocation block exists for a given page...
  static const auto has_reloc_page = [&](std::uint32_t page) -> bool {
    auto img = reinterpret_cast<win::image_t<>*>(output.data());
    auto sections = img->get_nt_headers()->get_sections();
    auto section_cnt = img->get_file_header()->num_sections;

    auto basereloc_dir =
        img->get_directory(win::directory_id::directory_entry_basereloc);
    auto reloc_dir = reinterpret_cast<win::reloc_directory_t*>(
        basereloc_dir->rva + output.data());
    win::reloc_block_t* reloc_block = &reloc_dir->first_block;

    while (reloc_block->base_rva && reloc_block->size_block) {
      if (reloc_block->base_rva == page)
        return true;

      reloc_block = reloc_block->next();
    }

    return false;
  };

  // calc size to add new reloc info...
  std::size_t resize_cnt = 0ull;
  for (const auto& [reloc_rva, relocs] : new_relocs)
    if (!has_reloc_page(reloc_rva))
      resize_cnt += sizeof(win::reloc_block_t) +
                    (relocs.size() * sizeof(win::reloc_entry_t));

  auto basereloc_dir =
      output_img->get_directory(win::directory_id::directory_entry_basereloc);

  resize_cnt += sizeof(win::reloc_block_t) + basereloc_dir->size;
  output.resize(output.size() + resize_cnt);
  output_img = reinterpret_cast<win::image_t<>*>(output.data());

  basereloc_dir =
      output_img->get_directory(win::directory_id::directory_entry_basereloc);

  auto reloc_dir = reinterpret_cast<win::reloc_directory_t*>(
      basereloc_dir->rva + output.data());

  // move .reloc section to the end of the file...
  std::for_each(
      output_img->get_nt_headers()->get_sections(),
      output_img->get_nt_headers()->get_sections() +
          output_img->get_nt_headers()->file_header.num_sections,
      [&](coff::section_header_t& hdr) {
        if (hdr.virtual_address == basereloc_dir->rva) {
          hdr.virtual_size = resize_cnt;
          hdr.size_raw_data = resize_cnt;
          hdr.virtual_address =
              output_img->get_nt_headers()->optional_header.size_image;
        }
      });

  // copy .reloc section data to the end of the file...
  std::memcpy(
      output.data() + output_img->get_nt_headers()->optional_header.size_image,
      output.data() + basereloc_dir->rva, basereloc_dir->size);

  // update directory data to reflect this .reloc move...
  basereloc_dir->rva = output_img->get_nt_headers()->optional_header.size_image;
  basereloc_dir->size = resize_cnt;

  // update size_image and make sure its page aligned... (round up...)
  output_img->get_nt_headers()->optional_header.size_image += resize_cnt;
  output_img->get_nt_headers()->optional_header.size_image =
      (output_img->get_nt_headers()->optional_header.size_image + PAGE_4KB) &
      ~0xFFFull;

  // add new relocs to the .reloc section...
  for (const auto& [reloc_rva, relocs] : new_relocs) {
    if (has_reloc_page(reloc_rva))
      continue;

    win::reloc_block_t* reloc_block = &reloc_dir->first_block;
    while (reloc_block->base_rva && reloc_block->size_block)
      reloc_block = reloc_block->next();

    reloc_block->base_rva = reloc_rva;
    reloc_block->size_block =
        relocs.size() * sizeof(win::reloc_entry_t) + sizeof(uint64_t);

    reloc_block->next()->base_rva = 0ull;
    reloc_block->next()->size_block = 0ull;

    for (auto idx = 0u; idx < relocs.size(); ++idx) {
      reloc_block->entries[idx].type = win::reloc_type_id::rel_based_dir64;
      reloc_block->entries[idx].offset = relocs[idx];
    }
  }
  return true;
}

void unpack_t::local_alloc_hook(uc_engine* uc_ctx, unpack_t* obj) {
  uc_err err;
  std::uintptr_t rax, rdx;

  if ((err = uc_reg_read(uc_ctx, UC_X86_REG_RDX, &rdx))) {
    std::printf("> failed to read RDX... reason = %d\n", rdx);
    return;
  }

  auto size = ((rdx + PAGE_4KB) & ~0xFFFull);
  if ((err = uc_mem_map(uc_ctx, HEAP_BASE + obj->heap_offset, size,
                        UC_PROT_ALL))) {
    std::printf("> failed to allocate memory... reason = %d\n", err);
    return;
  }

  std::printf("> LocalAlloc([ignore], %x)\n", rdx);

  rax = HEAP_BASE + obj->heap_offset;
  obj->heap_offset += size;

  if ((err = uc_reg_write(uc_ctx, UC_X86_REG_RAX, &rax))) {
    std::printf("> failed to write rax... reason = %d\n", err);
    return;
  }
}

void unpack_t::is_debugger_present_hook(uc_engine* uc, unpack_t* obj) {
  std::uintptr_t rax = 0ull;
  uc_reg_write(uc, UC_X86_REG_RAX, &rax);
  printf("> IsDebuggerPresent Called!\n");
}

void unpack_t::local_free_hook(uc_engine* uc_ctx, unpack_t* obj) {
  uc_err err;
  std::uintptr_t rax = 0ull;

  if ((err = uc_reg_write(uc_ctx, UC_X86_REG_RAX, &rax))) {
    std::printf("> failed to write rax... reason = %d\n", err);
    return;
  }
}

void unpack_t::unmap_view_of_file_hook(uc_engine* uc, unpack_t* obj) {
  std::uintptr_t rcx, rax = true;
  uc_reg_read(uc, UC_X86_REG_RCX, &rcx);
  uc_reg_write(uc, UC_X86_REG_RAX, &rax);
  std::printf("> UnmapViewOfFile(%p)\n", rcx);
}

void unpack_t::close_handle_hook(uc_engine* uc, unpack_t* obj) {
  std::uintptr_t rcx, rax = true;
  uc_reg_read(uc, UC_X86_REG_RCX, &rcx);
  uc_reg_write(uc, UC_X86_REG_RAX, &rax);
  std::printf("> CloseHandle(%x)\n", rcx);
}

void unpack_t::get_module_file_name_w_hook(uc_engine* uc, unpack_t* obj) {
  uc_err err;
  std::uintptr_t rcx, rdx, r8;
  uc_reg_read(uc, UC_X86_REG_RCX, &rcx);
  uc_reg_read(uc, UC_X86_REG_RDX, &rdx);
  uc_reg_read(uc, UC_X86_REG_R8, &r8);
  std::printf("> GetModuleFileNameW(%p, %p, %d)\n", rcx, rdx, r8);
  uc_strcpy(uc, rdx, (char*)obj->module_name.c_str());

  std::uint32_t size = obj->module_name.size();
  uc_reg_write(uc, UC_X86_REG_RAX, &size);
}

void unpack_t::create_file_w_hook(uc_engine* uc, unpack_t* obj) {
  char buff[256];
  std::uintptr_t rcx, rax = PACKED_FILE_HANDLE;
  uc_reg_read(uc, UC_X86_REG_RCX, &rcx);
  uc_strcpy(uc, buff, rcx);

  std::printf("> CreateFileW(%s)\n", buff);
  uc_reg_write(uc, UC_X86_REG_RAX, &rax);
}

void unpack_t::get_file_size_hook(uc_engine* uc, unpack_t* obj) {
  std::uintptr_t rcx, rdx, rax;
  uc_reg_read(uc, UC_X86_REG_RCX, &rcx);
  uc_reg_read(uc, UC_X86_REG_RDX, &rdx);

  std::printf("> GetFileSize(%x, %p)\n", rcx, rdx);
  if (rcx == PACKED_FILE_HANDLE) {
    rax = obj->bin.size();
    uc_reg_write(uc, UC_X86_REG_RAX, &rax);
  } else {
    std::printf("> asking for file size to unknown handle = %x\n", rcx);
    uc_emu_stop(uc);
  }
}

void unpack_t::create_file_mapping_hook(uc_engine* uc, unpack_t* obj) {
  std::uintptr_t rcx, r8, rax = PACKED_FILE_HANDLE;
  uc_reg_read(uc, UC_X86_REG_RCX, &rcx);
  uc_reg_read(uc, UC_X86_REG_R8, &r8);
  std::printf("> CreateFileMappingW(%x, %x)\n", rcx, r8);
  if (rcx == PACKED_FILE_HANDLE) {
    uc_reg_write(uc, UC_X86_REG_RAX, &rax);
  } else {
    std::printf("> asking to create mapping for unknown handle = %x\n", rcx);
    uc_emu_stop(uc);
  }
}

void unpack_t::map_view_of_file_hook(uc_engine* uc, unpack_t* obj) {
  std::uintptr_t rcx, rdx, r8, r9, rax;
  uc_reg_read(uc, UC_X86_REG_RCX, &rcx);
  uc_reg_read(uc, UC_X86_REG_RDX, &rdx);
  uc_reg_read(uc, UC_X86_REG_R8, &r8);
  uc_reg_read(uc, UC_X86_REG_R9, &r9);
  std::printf("> MapViewOfFile(%x, %x, %x, %x)\n", rcx, rdx, r8, r9);
  if (rcx == PACKED_FILE_HANDLE) {
    uc_err err;
    auto size = ((obj->bin.size() + PAGE_4KB) & ~0xFFFull);
    if ((err =
             uc_mem_map(uc, HEAP_BASE + obj->heap_offset, size, UC_PROT_ALL))) {
      std::printf("> failed to allocate memory... reason = %d\n", err);
      return;
    }

    rax = HEAP_BASE + obj->heap_offset;
    obj->heap_offset += size;

    if ((err = uc_mem_write(uc, rax, obj->bin.data(), obj->bin.size()))) {
      std::printf("> failed to map view of file... reason = %d\n", err);
      return;
    }

    if ((err = uc_reg_write(uc, UC_X86_REG_RAX, &rax))) {
      std::printf("> failed to write rax... reason = %d\n", err);
      return;
    }
  } else {
    std::printf("> asking to map file for unknown handle = %x\n", rcx);
    uc_emu_stop(uc);
  }
}

void unpack_t::virtual_protect_hook(uc_engine* uc, unpack_t* obj) {
  std::uintptr_t rcx, rdx, r8, r9, rax = true;
  uc_reg_read(uc, UC_X86_REG_RCX, &rcx);
  uc_reg_read(uc, UC_X86_REG_RDX, &rdx);
  uc_reg_read(uc, UC_X86_REG_R8, &r8);
  uc_reg_read(uc, UC_X86_REG_R9, &r9);
  std::printf("> VirtualProtect(%p, %x, %x, %p)\n", rcx, rdx, r8, r9);
  uc_reg_write(uc, UC_X86_REG_RAX, &rax);
}

void unpack_t::load_library_hook(uc_engine* uc_ctx, unpack_t* obj) {
  uc_err err;
  std::uintptr_t rcx = 0ull;

  if ((err = uc_reg_read(uc_ctx, UC_X86_REG_RCX, &rcx))) {
    std::printf("> uc_reg_read error, reason = %d\n", err);
    return;
  }

  char buff[256];
  uc_strcpy(uc_ctx, buff, rcx);
  std::printf("> LoadLibraryA(\"%s\")\n", buff);

  if (!obj->loaded_modules[buff]) {
    std::printf("> loading library from disk...\n");
    std::vector<std::uint8_t> module_data, tmp;
    if (!vm::util::open_binary_file(buff, module_data)) {
      std::printf(
          "> failed to open a dependency... please put %s in the same folder "
          "as vmemu...\n",
          buff);
      exit(-1);
    }

    auto img = reinterpret_cast<win::image_t<>*>(module_data.data());
    auto image_size = img->get_nt_headers()->optional_header.size_image;

    tmp.resize(image_size);
    std::memcpy(tmp.data(), module_data.data(), PAGE_4KB);
    std::for_each(img->get_nt_headers()->get_sections(),
                  img->get_nt_headers()->get_sections() +
                      img->get_nt_headers()->file_header.num_sections,
                  [&](const auto& section_header) {
                    std::memcpy(
                        tmp.data() + section_header.virtual_address,
                        module_data.data() + section_header.ptr_raw_data,
                        section_header.size_raw_data);
                  });

    const auto module_base = reinterpret_cast<std::uintptr_t>(tmp.data());
    img = reinterpret_cast<win::image_t<>*>(module_base);
    const auto image_base = img->get_nt_headers()->optional_header.image_base;
    const auto alloc_addr = module_base & ~0xFFFull;
    obj->loaded_modules[buff] = alloc_addr;

    auto dir = reinterpret_cast<win::import_directory_t*>(
        img->get_directory(win::directory_id::directory_entry_import));

    if (dir) {
      // install iat hooks...
      for (auto import_dir = reinterpret_cast<win::import_directory_t*>(
               img->get_directory(win::directory_id::directory_entry_import)
                   ->rva +
               module_base);
           import_dir->rva_name; ++import_dir) {
        for (auto iat_thunk = reinterpret_cast<win::image_thunk_data_t<>*>(
                 import_dir->rva_first_thunk + module_base);
             iat_thunk->address; ++iat_thunk) {
          if (iat_thunk->is_ordinal)
            continue;

          auto iat_name = reinterpret_cast<win::image_named_import_t*>(
              iat_thunk->address + module_base);

          if (obj->iat_hooks.find(iat_name->name) != obj->iat_hooks.end()) {
            std::printf(
                "> iat hooking %s to vector table %p\n", iat_name->name,
                obj->iat_hooks[iat_name->name].first + IAT_VECTOR_TABLE);

            iat_thunk->function =
                obj->iat_hooks[iat_name->name].first + IAT_VECTOR_TABLE;
          }
        }
      }
    }

    if ((err = uc_mem_map(uc_ctx, alloc_addr, image_size, UC_PROT_ALL))) {
      std::printf("> failed to load library... reason = %d\n", err);
      return;
    }

    if ((err = uc_mem_write(uc_ctx, alloc_addr, tmp.data(), image_size))) {
      std::printf("> failed to copy module into emulator... reason = %d\n",
                  err);
      return;
    }

    if ((err = uc_reg_write(uc_ctx, UC_X86_REG_RAX, &alloc_addr))) {
      std::printf("> failed to set rax... reason = %d\n", err);
      return;
    }
    std::printf("> mapped %s to base address %p\n", buff, alloc_addr);
  } else {
    const auto alloc_addr = obj->loaded_modules[buff];
    std::printf("> library already loaded... returning %p...\n", alloc_addr);
    if ((err = uc_reg_write(uc_ctx, UC_X86_REG_RAX, &alloc_addr))) {
      std::printf("> failed to set rax... reason = %d\n", err);
      return;
    }
  }
}  // namespace engine

void unpack_t::uc_strcpy(uc_engine* uc_ctx, char* buff, std::uintptr_t addr) {
  uc_err err;
  char i = 0u;
  auto idx = 0ul;

  do {
    if ((err = uc_mem_read(uc_ctx, addr + idx, &i, sizeof i))) {
      std::printf("[!] error reading string byte... reason = %d\n", err);
      break;
    }
  } while ((buff[idx++] = i));
}

void unpack_t::uc_strcpy(uc_engine* uc, std::uintptr_t addr, char* buff) {
  uc_err err;
  for (char idx = 0u, c = buff[idx]; buff[idx]; ++idx, c = buff[idx]) {
    if ((err = uc_mem_write(uc, addr + idx, &c, sizeof c))) {
      std::printf("[!] error writing string byte... reason = %d\n", err);
      break;
    }
  }
}

bool unpack_t::iat_dispatcher(uc_engine* uc,
                              uint64_t address,
                              uint32_t size,
                              unpack_t* unpack) {
  auto vec = address - IAT_VECTOR_TABLE;
  for (auto& [iat_name, iat_hook_data] : unpack->iat_hooks) {
    if (iat_hook_data.first == vec) {
      iat_hook_data.second(uc, unpack);
      return true;
    }
  }
  return false;
}

void unpack_t::invalid_mem(uc_engine* uc,
                           uc_mem_type type,
                           uint64_t address,
                           int size,
                           int64_t value,
                           unpack_t* unpack) {
  switch (type) {
    case UC_MEM_READ_UNMAPPED: {
      uc_mem_map(uc, address & ~0xFFFull, PAGE_4KB, UC_PROT_ALL);
      std::printf(">>> reading invalid memory at address = %p, size = 0x%x\n",
                  address, size);
      break;
    }
    case UC_MEM_WRITE_UNMAPPED: {
      uc_mem_map(uc, address & ~0xFFFull, PAGE_4KB, UC_PROT_ALL);
      std::uintptr_t rip = 0;
      uc_reg_read(uc, UC_X86_REG_RIP, &rip);

      std::printf(
          ">>> writing invalid memory at address = %p, size = 0x%x, val = "
          "0x%x rip = %p\n",
          address, size, value, rip);
      break;
    }
    case UC_MEM_FETCH_UNMAPPED: {
      std::printf(">>> fetching invalid instructions at address = %p\n",
                  address);
      break;
    }
    default:
      break;
  }
}
}  // namespace engine