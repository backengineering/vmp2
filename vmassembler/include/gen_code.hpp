#pragma once
#include <compiler.hpp>
#include <fstream>
#include <vmprofiler.hpp>
#include <xtils.hpp>

namespace gen {
/// <summary>
/// function pasted from
/// https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
/// </summary>
/// <typeparam name="...Args"></typeparam>
/// <param name="format"></param>
/// <param name="...args"></param>
/// <returns></returns>
template <typename... Args>
std::string string_format(const std::string& format, Args... args) {
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
  auto size = static_cast<size_t>(size_s);
  auto buf = std::make_unique<char[]>(size);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1);
}

/// <summary>
/// generates c++ header file which MUST BE COMPILED USING CLANG BECAUSE MSVC
/// CANT HANDLE HUGE STATIC INITS (6/13/2021)....
/// </summary>
/// <param name="labels">vector of compiled labels...</param>
/// <param name="image_path">path to the image on disk...</param>
/// <param name="vmctx"></param>
/// <returns></returns>
inline std::string code(std::vector<vm::compiled_label_data>& labels,
                        std::string image_path,
                        vm::ctx_t& vmctx) {
  std::string result, raw_file_array;
  std::vector<std::uint8_t> raw_file;
  xtils::um_t::get_instance()->open_binary_file(image_path, raw_file);

  for (auto idx = 0u; idx < raw_file.size(); ++idx)
    raw_file_array.append(string_format("0x%x, ", raw_file[idx]));

  result.append(
      "#pragma once\n"
      "#pragma section( \".xmp2\" )\n"
      "#pragma comment( linker, \"/section:.xmp2,RWE\" ) \n\n");

  result.append(
      "namespace vm\n"
      "{\n");

  result.append(R"(    using u8 = unsigned char;
    using s8 = signed char;

    using u16 = unsigned short;
    using s16 = signed short;

    using u32 = unsigned int;
    using s32 = signed int;

    using u64 = unsigned long long;
    using s64 = signed long long;
    using __vmcall_t = void* (*)(...);
        
    constexpr u8 IMAGE_DIRECTORY_ENTRY_BASERELOC = 5;
    constexpr u8 IMAGE_REL_BASED_ABSOLUTE = 0;
    constexpr u8 IMAGE_REL_BASED_DIR64 = 10;

    typedef struct _IMAGE_DOS_HEADER
    {
        /* 0x0000 */ unsigned short e_magic;
        /* 0x0002 */ unsigned short e_cblp;
        /* 0x0004 */ unsigned short e_cp;
        /* 0x0006 */ unsigned short e_crlc;
        /* 0x0008 */ unsigned short e_cparhdr;
        /* 0x000a */ unsigned short e_minalloc;
        /* 0x000c */ unsigned short e_maxalloc;
        /* 0x000e */ unsigned short e_ss;
        /* 0x0010 */ unsigned short e_sp;
        /* 0x0012 */ unsigned short e_csum;
        /* 0x0014 */ unsigned short e_ip;
        /* 0x0016 */ unsigned short e_cs;
        /* 0x0018 */ unsigned short e_lfarlc;
        /* 0x001a */ unsigned short e_ovno;
        /* 0x001c */ unsigned short e_res[ 4 ];
        /* 0x0024 */ unsigned short e_oemid;
        /* 0x0026 */ unsigned short e_oeminfo;
        /* 0x0028 */ unsigned short e_res2[ 10 ];
        /* 0x003c */ long e_lfanew;
    } IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER; /* size: 0x0040 */

    typedef struct _IMAGE_FILE_HEADER
    {
        /* 0x0000 */ unsigned short Machine;
        /* 0x0002 */ unsigned short NumberOfSections;
        /* 0x0004 */ unsigned long TimeDateStamp;
        /* 0x0008 */ unsigned long PointerToSymbolTable;
        /* 0x000c */ unsigned long NumberOfSymbols;
        /* 0x0010 */ unsigned short SizeOfOptionalHeader;
        /* 0x0012 */ unsigned short Characteristics;
    } IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER; /* size: 0x0014 */

    typedef struct _IMAGE_DATA_DIRECTORY
    {
        /* 0x0000 */ unsigned long VirtualAddress;
        /* 0x0004 */ unsigned long Size;
    } IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY; /* size: 0x0008 */

    typedef struct _IMAGE_OPTIONAL_HEADER64
    {
        /* 0x0000 */ unsigned short Magic;
        /* 0x0002 */ unsigned char MajorLinkerVersion;
        /* 0x0003 */ unsigned char MinorLinkerVersion;
        /* 0x0004 */ unsigned long SizeOfCode;
        /* 0x0008 */ unsigned long SizeOfInitializedData;
        /* 0x000c */ unsigned long SizeOfUninitializedData;
        /* 0x0010 */ unsigned long AddressOfEntryPoint;
        /* 0x0014 */ unsigned long BaseOfCode;
        /* 0x0018 */ unsigned __int64 ImageBase;
        /* 0x0020 */ unsigned long SectionAlignment;
        /* 0x0024 */ unsigned long FileAlignment;
        /* 0x0028 */ unsigned short MajorOperatingSystemVersion;
        /* 0x002a */ unsigned short MinorOperatingSystemVersion;
        /* 0x002c */ unsigned short MajorImageVersion;
        /* 0x002e */ unsigned short MinorImageVersion;
        /* 0x0030 */ unsigned short MajorSubsystemVersion;
        /* 0x0032 */ unsigned short MinorSubsystemVersion;
        /* 0x0034 */ unsigned long Win32VersionValue;
        /* 0x0038 */ unsigned long SizeOfImage;
        /* 0x003c */ unsigned long SizeOfHeaders;
        /* 0x0040 */ unsigned long CheckSum;
        /* 0x0044 */ unsigned short Subsystem;
        /* 0x0046 */ unsigned short DllCharacteristics;
        /* 0x0048 */ unsigned __int64 SizeOfStackReserve;
        /* 0x0050 */ unsigned __int64 SizeOfStackCommit;
        /* 0x0058 */ unsigned __int64 SizeOfHeapReserve;
        /* 0x0060 */ unsigned __int64 SizeOfHeapCommit;
        /* 0x0068 */ unsigned long LoaderFlags;
        /* 0x006c */ unsigned long NumberOfRvaAndSizes;
        /* 0x0070 */ struct _IMAGE_DATA_DIRECTORY DataDirectory[ 16 ];
    } IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64; /* size: 0x00f0 */

    typedef struct _IMAGE_NT_HEADERS64
    {
        /* 0x0000 */ unsigned long Signature;
        /* 0x0004 */ struct _IMAGE_FILE_HEADER FileHeader;
        /* 0x0018 */ struct _IMAGE_OPTIONAL_HEADER64 OptionalHeader;
    } IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64; /* size: 0x0108 */

    typedef struct _IMAGE_SECTION_HEADER
    {
        /* 0x0000 */ unsigned char Name[ 8 ];
        union
        {
            union
            {
                /* 0x0008 */ unsigned long PhysicalAddress;
                /* 0x0008 */ unsigned long VirtualSize;
            }; /* size: 0x0004 */
        } /* size: 0x0004 */ Misc;
        /* 0x000c */ unsigned long VirtualAddress;
        /* 0x0010 */ unsigned long SizeOfRawData;
        /* 0x0014 */ unsigned long PointerToRawData;
        /* 0x0018 */ unsigned long PointerToRelocations;
        /* 0x001c */ unsigned long PointerToLinenumbers;
        /* 0x0020 */ unsigned short NumberOfRelocations;
        /* 0x0022 */ unsigned short NumberOfLinenumbers;
        /* 0x0024 */ unsigned long Characteristics;
    } IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER; /* size: 0x0028 */

    typedef struct _IMAGE_BASE_RELOCATION
    {
        unsigned int VirtualAddress;
        unsigned int SizeOfBlock;
    } IMAGE_BASE_RELOCATION, *PIMAGE_BASE_RELOCATION;)");

  result.append(
      "\n\n\ttemplate < class T, class U > struct _pair_t\n"
      "\t{\n"
      "\t\tT first;\n"
      "\t\tU second;\n"
      "\t};\n\n");

  result.append(string_format("\tconstexpr auto entry_rva = 0x%x;\n\n",
                              vmctx.vm_entry_rva));

  result.append(
      "\tenum class calls : u32\n"
      "\t{\n");

  for (auto idx = 0u; idx < labels.size(); ++idx)
    result.append(string_format("\t\t%s = 0x%x,\n", labels[idx].label_name,
                                labels[idx].enc_alloc_rva));

  result.append("\t};\n\n");
  result.append(
      string_format("\tinline _pair_t< u8, calls > call_map[%d] = \n"
                    "\t{\n",
                    labels.size()));

  for (auto idx = 0u; idx < labels.size(); ++idx)
    result.append(string_format("\t\t{ %d, calls::%s },\n", idx,
                                labels[idx].label_name.c_str()));

  result.append("\t};\n\n");
  result.append(
      string_format("\t__declspec(align(1)) struct _gen_data\n"
                    "\t{\n"
                    "\t\tu8 bin[%d] =\n"
                    "\t\t{\n"
                    "\t\t\t%s\n",
                    raw_file.size(), raw_file_array.c_str()));

  result.append("\t\t};\n\n");
  result.append(string_format("\t\tu8 map_area[0x%x];\n\n", vmctx.image_size));

  for (auto& label : labels) {
    result.append(string_format("\t\tu8 __%s_vinstrs[%d] =\n",
                                label.label_name.c_str(),
                                label.vinstrs.size()));
    result.append("\t\t{\n\t\t\t");
    for (auto& byte : label.vinstrs)
      result.append(string_format("0x%x, ", byte));
    result.append("\n\t\t};\n\n");
  }

  result.append(
      string_format("\t\tu8 __vmcall_shell_code[%d][15] =\n"
                    "\t\t{\n",
                    labels.size()));

  for (auto idx = 0u; idx < labels.size(); ++idx) {
    std::string jmp_code;

    // two push instructions...
    for (auto i = 0u; i < 2; ++i) {
      jmp_code.append("0x68, ");  // push opcode...

      for (auto _idx = 0u; _idx < 4; ++_idx)
        jmp_code.append("0x0, ");
    }

    // one jmp instruction...
    jmp_code.append("0xE9, ");
    for (auto i = 0u; i < 4; ++i)
      jmp_code.append("0x0, ");

    result.append(string_format("\t\t\t{ %s },\n", jmp_code.c_str()));
  }

  result.append("\t\t};\n\n");
  result.append(R"(        bool init()
        {
            static const auto _memcpy = []( void *dest, const void *src, size_t len ) -> void * {
                char *d = ( char * )dest;
                const char *s = ( char * )src;
                while ( len-- )
                    *d++ = *s++;

                return dest;
            };

            const auto dos_header = reinterpret_cast< IMAGE_DOS_HEADER * >( bin );
            const auto nt_headers = reinterpret_cast< PIMAGE_NT_HEADERS64 >( bin + dos_header->e_lfanew );

            _memcpy( map_area, bin, nt_headers->OptionalHeader.SizeOfHeaders );

            auto sections = reinterpret_cast< PIMAGE_SECTION_HEADER >( ( u8 * )&nt_headers->OptionalHeader +
                                                                        nt_headers->FileHeader.SizeOfOptionalHeader );

            // map sections...
            for ( u32 i = 0; i < nt_headers->FileHeader.NumberOfSections; ++i )
            {
                PIMAGE_SECTION_HEADER section = &sections[ i ];

                _memcpy( ( void * )( map_area + section->VirtualAddress ),
                            ( void * )( bin + section->PointerToRawData ), section->SizeOfRawData );
            }

            // handle relocations...
            const auto reloc_dir = &nt_headers->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ];

            if ( reloc_dir->VirtualAddress )
            {
                auto reloc = reinterpret_cast< IMAGE_BASE_RELOCATION * >( map_area + reloc_dir->VirtualAddress );

                for ( auto current_size = 0u; current_size < reloc_dir->Size; )
                {
                    u32 reloc_count = ( reloc->SizeOfBlock - sizeof( IMAGE_BASE_RELOCATION ) ) / sizeof( u16 );
                    u16 *reloc_data = ( u16 * )( ( u8 * )reloc + sizeof( IMAGE_BASE_RELOCATION ) );
                    u8 *reloc_base = map_area + reloc->VirtualAddress;

                    for ( auto idx = 0; idx < reloc_count; ++idx, ++reloc_data )
                    {
                        u16 data = *reloc_data;
                        u16 type = data >> 12;
                        u16 offset = data & 0xFFF;

                        switch ( type )
                        {
                        case IMAGE_REL_BASED_ABSOLUTE:
                            break;
                        case IMAGE_REL_BASED_DIR64:
                        {
                            u64 *rva = ( u64 * )( reloc_base + offset );
                            *rva = ( u64 )( map_area + ( *rva - nt_headers->OptionalHeader.ImageBase ) );
                            break;
                        }
                        default:
                            break;
                        }
                    }

                    current_size += reloc->SizeOfBlock;
                    reloc = ( IMAGE_BASE_RELOCATION * )reloc_data;
                }
            }

            // fix shellcode up...
            for ( auto idx = 0u; idx < ( sizeof( __vmcall_shell_code ) / 15 ); ++idx )
            {
                // first push encrypted rva value...
                *reinterpret_cast< u32 * >( &__vmcall_shell_code[ idx ][ 1 ] ) =
                    static_cast< u32 >( call_map[ idx ].second );

                // second push encrypted rva value...
                *reinterpret_cast< u32 * >( &__vmcall_shell_code[ idx ][ 6 ] ) =
                    static_cast< u32 >( call_map[ idx ].second );

                // signed rip relative rva to vm entry...
                *reinterpret_cast< u32 * >( &__vmcall_shell_code[ idx ][ 11 ] ) = reinterpret_cast< s32 >(
                    ( map_area - ( reinterpret_cast< u64 >( &__vmcall_shell_code[ idx ] ) + 15 ) ) + entry_rva );
            }

            return true; // only a bool so i can use static/call init only once...
        })");

  result.append("\n\t};\n\n");
  result.append(
      "\t__declspec(allocate(\".xmp2\")) inline _gen_data gen_data;\n\n");

  result.append(
      R"(    template < calls e_call, class T, class ... Ts > auto call(const Ts... args) -> T
    {
        static auto __init_result = gen_data.init();

        __vmcall_t vmcall = nullptr;
        for ( auto idx = 0u; idx < sizeof( call_map ) / sizeof( _pair_t< u8, calls > ); ++idx )
            if ( call_map[ idx ].second == e_call )
                vmcall = reinterpret_cast< __vmcall_t >( &gen_data.__vmcall_shell_code[ idx ] );

        return reinterpret_cast< T >( vmcall( args... ) );
    })");

  result.append("\n}");
  return result;
}
}  // namespace gen