#include <devirt_utils.hpp>

namespace devirt
{
    namespace util
    {
        bool serialize_vmp2(
            std::vector< std::pair< std::uint32_t, std::vector< vm::instrs::code_block_t > > > &virt_rtns,
            std::vector< std::uint8_t > &vmp2file )
        {
            const auto file_header = reinterpret_cast< vmp2::v4::file_header * >( vmp2file.data() );

            if ( file_header->version != vmp2::version_t::v4 )
            {
                std::printf( "[!] invalid vmp2 file version... this build uses v4...\n" );
                return false;
            }

            auto first_rtn = reinterpret_cast< vmp2::v4::rtn_t * >( reinterpret_cast< std::uintptr_t >( file_header ) +
                                                                    file_header->rtn_offset );

            for ( auto [ rtn_block, rtn_idx ] = std::pair{ first_rtn, 0ull }; rtn_idx < file_header->rtn_count;
                  ++rtn_idx, rtn_block = reinterpret_cast< vmp2::v4::rtn_t * >(
                                 reinterpret_cast< std::uintptr_t >( rtn_block ) + rtn_block->size ) )
            {
                virt_rtns.push_back( { rtn_block->vm_enter_offset, {} } );
                for ( auto [ code_block, block_idx ] = std::pair{ &rtn_block->code_blocks[ 0 ], 0ull };
                      block_idx < rtn_block->code_block_count;
                      ++block_idx, code_block = reinterpret_cast< vmp2::v4::code_block_t * >(
                                       reinterpret_cast< std::uintptr_t >( code_block ) +
                                       code_block->next_block_offset ) )
                {
                    auto block_vinstrs = reinterpret_cast< vm::instrs::virt_instr_t * >(
                        reinterpret_cast< std::uintptr_t >( code_block ) + sizeof vmp2::v4::code_block_t +
                        ( code_block->num_block_addrs * 8 ) );

                    vm::instrs::code_block_t _code_block{ code_block->vip_begin };
                    _code_block.jcc.has_jcc = code_block->has_jcc;
                    _code_block.jcc.type = code_block->jcc_type;

                    for ( auto idx = 0u; idx < code_block->num_block_addrs; ++idx )
                        _code_block.jcc.block_addr.push_back( code_block->branch_addr[ idx ] );

                    for ( auto idx = 0u; idx < code_block->vinstr_count; ++idx )
                        _code_block.vinstrs.push_back( block_vinstrs[ idx ] );

                    virt_rtns.back().second.push_back( _code_block );
                }
            }

            return true;
        }
    } // namespace util

    bool append( std::vector< std::uint8_t > &obj, std::vector< std::uint8_t > &bin )
    {
        if ( obj.empty() || bin.empty() )
            return false;

        std::vector< std::uint8_t > map_buff;
        auto bin_img = reinterpret_cast< win::image_t<> * >( bin.data() );

        // increase the size_image by the size of the obj as the entire obj will be in the .devirt section...
        map_buff.resize(
            reinterpret_cast< win::image_t<> * >( bin.data() )->get_nt_headers()->optional_header.size_image +=
            obj.size() );

        // copy over dos headers, pe headers (section headers, optional header etc)...
        std::memcpy( map_buff.data(), bin.data(), bin_img->get_nt_headers()->optional_header.size_headers );

        // map sections into map_img... also make image offset == virtual offset...
        auto map_img = reinterpret_cast< win::image_t<> * >( map_buff.data() );
        std::for_each( bin_img->get_nt_headers()->get_sections(),
                       bin_img->get_nt_headers()->get_sections() + bin_img->get_file_header()->num_sections,
                       [ & ]( coff::section_header_t &section )
                       {
                           if ( !section.virtual_address || !section.ptr_raw_data )
                               return;

                           std::memcpy( map_buff.data() + section.virtual_address, bin.data() + section.ptr_raw_data,
                                        section.size_raw_data );
                       } );

        std::for_each( map_img->get_nt_headers()->get_sections(),
                       map_img->get_nt_headers()->get_sections() + bin_img->get_file_header()->num_sections,
                       [ & ]( coff::section_header_t &section )
                       {
                           section.ptr_raw_data = section.virtual_address;
                           section.size_raw_data = section.virtual_size;
                       } );

        auto pe_sections = map_img->get_nt_headers()->get_sections();
        auto num_sections = map_img->get_file_header()->num_sections;

        // append .devirt section to the original binary...
        strcpy( pe_sections[ num_sections ].name.short_name, ".devirt" );
        pe_sections[ num_sections ].virtual_size = obj.size();
        pe_sections[ num_sections ].size_raw_data = obj.size();
        pe_sections[ num_sections ].virtual_address = ( pe_sections[ num_sections - 1 ].virtual_address +
                                                        pe_sections[ num_sections - 1 ].virtual_size + 0x1000 ) &
                                                      ~0xFFFull;
        pe_sections[ num_sections ].ptr_raw_data = pe_sections[ num_sections ].virtual_address;

        coff::section_characteristics_t prots;
        prots.flags = 0ull;
        prots.mem_execute = true;
        prots.mem_read = true;
        pe_sections[ num_sections ].characteristics = prots;
        ++map_img->get_file_header()->num_sections;
        std::memcpy( map_buff.data() + pe_sections[ num_sections ].virtual_address, obj.data(), obj.size() );

        auto obj_img_addr = map_buff.data() + pe_sections[ num_sections ].virtual_address;
        auto obj_img = reinterpret_cast< coff::image_t * >( obj_img_addr );
        auto str_table = obj_img->get_strings();
        auto symbols = obj_img->get_symbols();
        auto obj_sections = obj_img->get_sections();
        auto symbol_cnt = obj_img->file_header.num_symbols;

        static const auto get_symbol = [ & ]( std::string symbol_name ) -> auto
        {
            return std::find_if( symbols, symbols + symbol_cnt,
                                 [ & ]( coff::symbol_t &symbol )
                                 {
                                     if ( symbol.derived_type != coff::derived_type_id::function )
                                         return false;

                                     return !strcmp( symbol.name.to_string( str_table ).data(), symbol_name.c_str() );
                                 } );
        };

        std::vector< std::pair< std::uint32_t, std::uint16_t > > new_relocs;
        std::for_each(
            symbols, symbols + symbol_cnt,
            [ & ]( coff::symbol_t &symbol )
            {
                if ( symbol.derived_type != coff::derived_type_id::function )
                    return;

                auto symbol_name = symbol.name.to_string( str_table );
                printf( "Symbol Name: %s\n", symbol_name.data() );
                if ( strstr( symbol_name.data(), VM_ENTER_NAME ) )
                {
                    // fix 0x1000000000000000 to 0x0000000000000000
                    // also push back a new relocation for this entry...
                    auto symbol_addr = reinterpret_cast< std::uintptr_t >( obj_img_addr ) + symbol.value +
                                       obj_sections[ symbol.section_index - 1 ].ptr_raw_data;

                    *reinterpret_cast< std::uintptr_t * >( symbol_addr + FIX_MAKE_ZERO_OFFSET ) = 0ull;
                    *reinterpret_cast< std::uintptr_t * >( symbol_addr + FIX_MAKE_RELOC_OFFSET ) = 0ull;

                    auto page = ( symbol.value + obj_sections[ symbol.section_index - 1 ].ptr_raw_data +
                                  FIX_MAKE_RELOC_OFFSET + pe_sections[ num_sections ].virtual_address ) &
                                ~0xFFFull;

                    auto offset = ( symbol.value + obj_sections[ symbol.section_index - 1 ].ptr_raw_data +
                                    FIX_MAKE_RELOC_OFFSET + pe_sections[ num_sections ].virtual_address ) &
                                  0xFFFull;

                    new_relocs.push_back( { page, offset } );

                    // look up the rtn_xxxxxx function symbol for this vm enter and make it jmp to it...
                    auto rtn_offset = std::stoull( symbol_name.data() + sizeof( VM_ENTER_NAME ) - 1, nullptr, 16 );

                    std::stringstream rtn_name;
                    rtn_name << "rtn_" << std::hex << rtn_offset;
                    auto rtn_sym = get_symbol( rtn_name.str() );
                    auto relocs = reinterpret_cast< coff::reloc_t * >(
                        obj_sections[ rtn_sym->section_index - 1 ].ptr_relocs + obj_img_addr );

                    auto rtn_rva = rtn_offset - map_img->get_nt_headers()->optional_header.image_base;
                    std::int32_t devirt_rtn_rva = symbol_addr - reinterpret_cast< std::uintptr_t >( map_buff.data() );

                    *reinterpret_cast< std::int32_t * >( rtn_rva + map_buff.data() + 1 ) =
                        devirt_rtn_rva - ( rtn_rva + 5 );
                    *reinterpret_cast< std::uint8_t * >( rtn_rva + map_buff.data() ) = 0xE9;

                    printf( "Patching rva %x with jmp\n", rtn_rva );

                    // apply relocations to the rtn_xxxxx...
                    for ( auto reloc_idx = 0u; reloc_idx < obj_sections[ rtn_sym->section_index - 1 ].num_relocs;
                          ++reloc_idx )
                    {
                        coff::reloc_t &reloc = relocs[ reloc_idx ];

                        auto vmexit_sym =
                            get_symbol( std::string( symbols[ reloc.symbol_index ].name.to_string( str_table ) ) );

                        auto vmexit_offset =
                            vmexit_sym->value + obj_sections[ vmexit_sym->section_index - 1 ].ptr_raw_data;

                        auto reloc_file_offset =
                            reloc.virtual_address + obj_sections[ rtn_sym->section_index - 1 ].ptr_raw_data;

                        std::int32_t rva = vmexit_offset - ( std::int32_t )( reloc_file_offset + 4 );
                        *reinterpret_cast< std::int32_t * >( obj_img_addr + reloc_file_offset ) = rva;
                    }

                    auto rtn_addr = reinterpret_cast< std::uintptr_t >(
                        obj_img_addr + obj_sections[ rtn_sym->section_index - 1 ].ptr_raw_data + rtn_sym->value );

                    // create jmp to the rtn_xxxxx....
                    std::int32_t rva = rtn_addr - ( std::int64_t )( symbol_addr + FIX_MAKE_JMP_OFFSET + 5 );
                    *reinterpret_cast< std::uint8_t * >( symbol_addr + FIX_MAKE_JMP_OFFSET ) = 0xE9;
                    *reinterpret_cast< std::int32_t * >( symbol_addr + FIX_MAKE_JMP_OFFSET + 1 ) = rva;
                }
            } );

        // replace bin vector with map_buff vector...
        bin.clear();
        bin.insert( bin.begin(), map_buff.begin(), map_buff.end() );
        return true;
    }
} // namespace devirt