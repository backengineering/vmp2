#pragma once
#include <Zydis/Zydis.h>
#include <unicorn/unicorn.h>

#include <atomic>
#include <fstream>
#include <functional>
#include <map>
#include <nt/image.hpp>
#include <vector>
#include <vmprofiler.hpp>

#define PACKED_FILE_HANDLE 0x69420
#define PAGE_4KB 0x1000
#define STACK_SIZE PAGE_4KB * 512

#define IAT_VECTOR_TABLE 0x600000000000
#define STACK_BASE 0x7000000000000
#define HEAP_BASE 0x800000000000

#define EX_ALLOCATE_POOL_VECTOR 0
#define EX_FREE_POOL_VECTOR 1
#define LOCAL_ALLOC_VECTOR 2
#define LOCAL_FREE_VECTOR 3
#define LOAD_LIBRARY_VECTOR 4
#define NT_QUERY_SYSTEM_INFO_VECTOR 5
#define GET_MODULE_FILE_NAME_W_VECTOR 6
#define CREATE_FILE_W_VECTOR 7
#define GET_FILE_SIZE_VECTOR 8
#define CREATE_FILE_MAPPING_VECTOR 9
#define MAP_VIEW_OF_FILE_VECTOR 10
#define UNMAP_VIEW_OF_FILE_VECTOR 11
#define CLOSE_HANDLE_VECTOR 12
#define VIRTUAL_PROTECT_VECTOR 13
#define IS_DEBUGGER_PRESENT_VECTOR 14
#define IS_REMOTE_DEBUGGER_PRESENT_VECTOR 15

#define MOV_RAX_0_SIG "\x48\xB8\x00\x00\x00\x00\x00\x00\x00\x00"
#define MOV_RAX_0_MASK "xxxxxxxxxx"

static_assert(sizeof MOV_RAX_0_SIG == sizeof MOV_RAX_0_MASK,
              "signature and mask sizes are wrong...");

namespace engine {
class unpack_t {
 public:
  explicit unpack_t(const std::string& module_name,
                    const std::vector<std::uint8_t>& bin);
  ~unpack_t(void);

  bool init(void);
  bool unpack(std::vector<std::uint8_t>& output);

 private:
  using iat_hook_t = std::function<void(uc_engine*, unpack_t*)>;

  uc_engine* uc_ctx;
  std::vector<uint8_t> bin, map_bin;
  std::vector<uc_hook*> uc_hooks;
  std::string module_name;

  std::uintptr_t img_base, img_size, heap_offset, pack_section_offset;
  win::image_t<>* win_img;

  static void local_alloc_hook(uc_engine*, unpack_t*);
  static void local_free_hook(uc_engine*, unpack_t*);
  static void load_library_hook(uc_engine*, unpack_t*);
  static void get_module_file_name_w_hook(uc_engine*, unpack_t*);
  static void create_file_w_hook(uc_engine*, unpack_t*);
  static void get_file_size_hook(uc_engine*, unpack_t*);
  static void create_file_mapping_hook(uc_engine*, unpack_t*);
  static void map_view_of_file_hook(uc_engine*, unpack_t*);
  static void unmap_view_of_file_hook(uc_engine*, unpack_t*);
  static void close_handle_hook(uc_engine*, unpack_t*);
  static void virtual_protect_hook(uc_engine*, unpack_t*);
  static void is_debugger_present_hook(uc_engine*, unpack_t*);

  static void uc_strcpy(uc_engine* uc, char* buff, std::uintptr_t addr);
  static void uc_strcpy(uc_engine* uc, std::uintptr_t addr, char* buff);

  static bool iat_dispatcher(uc_engine* uc,
                             uint64_t address,
                             uint32_t size,
                             unpack_t* unpack);

  static void invalid_mem(uc_engine* uc,
                          uc_mem_type type,
                          uint64_t address,
                          int size,
                          int64_t value,
                          unpack_t* unpack);

  std::map<std::string, std::uintptr_t> loaded_modules;
  std::map<std::string, std::pair<std::uint32_t, iat_hook_t> > iat_hooks = {
      {"LocalAlloc", {LOCAL_ALLOC_VECTOR, &local_alloc_hook}},
      {"LocalFree", {LOCAL_FREE_VECTOR, &local_free_hook}},
      {"LoadLibraryA", {LOAD_LIBRARY_VECTOR, &load_library_hook}},
      {"GetModuleFileNameW",
       {GET_MODULE_FILE_NAME_W_VECTOR, &get_module_file_name_w_hook}},
      {"CreateFileW", {CREATE_FILE_W_VECTOR, &create_file_w_hook}},
      {"GetFileSize", {GET_FILE_SIZE_VECTOR, &get_file_size_hook}},
      {"CreateFileMappingW",
       {CREATE_FILE_MAPPING_VECTOR, &create_file_mapping_hook}},
      {"MapViewOfFile", {MAP_VIEW_OF_FILE_VECTOR, &map_view_of_file_hook}},
      {"UnmapViewOfFile",
       {UNMAP_VIEW_OF_FILE_VECTOR, &unmap_view_of_file_hook}},
      {"CloseHandle", {CLOSE_HANDLE_VECTOR, &close_handle_hook}},
      {"VirtualProtect", {VIRTUAL_PROTECT_VECTOR, &virtual_protect_hook}},
      {"IsDebuggerPresent",
       {IS_DEBUGGER_PRESENT_VECTOR, &is_debugger_present_hook}},
      {"CheckRemoteDebuggerPresent",
       {IS_REMOTE_DEBUGGER_PRESENT_VECTOR, &is_debugger_present_hook}}};
};
}  // namespace engine