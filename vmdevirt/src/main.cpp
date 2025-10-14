#include <Windows.h>
#include <argparse.h>
#include <devirt_t.hpp>
#include <devirt_utils.hpp>
#include <xtils.hpp>

using namespace argparse;

int main( int argc, const char *argv[] )
{
    ArgumentParser parser( "vmdevirt", "virtual instruction pseudo code generator" );
    parser.add_argument().name( "--vmp2file" ).required( true ).description( "path to .vmp2 file..." );
    parser.add_argument().name( "--out" ).required( true ).description( "output devirtualized binary name..." );
    parser.add_argument().name( "--genobj" ).description( "write the generated obj file to disk..." );
    parser.add_argument().name( "--bin" ).required( true ).description(
        "path to the image in which to apply devirtualized code too...\n" );

    parser.enable_help();
    auto err = parser.parse( argc, argv );

    if ( err )
    {
        std::cout << err << std::endl;
        return -1;
    }

    if ( parser.exists( "help" ) )
    {
        parser.print_help();
        return 0;
    }

    std::vector< std::uint8_t > vmp2file, bin;
    const auto umtils = xtils::um_t::get_instance();

    if ( !umtils->open_binary_file( parser.get< std::string >( "vmp2file" ), vmp2file ) )
    {
        std::printf( "[!] failed to open vmp2 file...\n" );
        return -1;
    }

    if ( !umtils->open_binary_file( parser.get< std::string >( "bin" ), bin ) )
    {
        std::printf( "[!] failed to open original binary file...\n" );
        return -1;
    }

    const auto file_header = reinterpret_cast< vmp2::v4::file_header * >( vmp2file.data() );
    std::vector< std::pair< std::uint32_t, std::vector< vm::instrs::code_block_t > > > virt_rtns;

    if ( !devirt::util::serialize_vmp2( virt_rtns, vmp2file ) )
    {
        std::printf( "> failed to serialize vmp2 file...\n" );
        return false;
    }

    llvm::LLVMContext llvm_ctx;
    llvm::Module llvm_module( "VMProtect 2 Devirtualization", llvm_ctx );

    std::vector< std::uint8_t > compiled_obj;
    vm::devirt_t vmp_devirt( &llvm_ctx, &llvm_module, file_header );

    for ( auto &[ vm_enter_offset, vmp2_code_blocks ] : virt_rtns )
    {
        if ( vmp2_code_blocks.empty() )
            continue;

        if ( !vmp_devirt.lift( vm_enter_offset + file_header->image_base, vmp2_code_blocks ) )
        {
            std::printf( "[!] failed to lift rtn_0x%p, please review the console...\n",
                         vm_enter_offset + file_header->image_base );

            return -1;
        }

        std::printf( "> lifted rtn_0x%p\n", vm_enter_offset + file_header->image_base );
    }

    llvm::LLVMInitializeX86TargetInfo();
    llvm::LLVMInitializeX86Target();
    llvm::LLVMInitializeX86TargetMC();
    llvm::LLVMInitializeX86AsmParser();
    llvm::LLVMInitializeX86AsmPrinter();

    vmp_devirt.compile( compiled_obj );
    if ( parser.exists( "genobj" ) )
        std::ofstream( parser.get< std::string >( "out" ).append( ".o" ), std::ios::binary )
            .write( reinterpret_cast< const char * >( compiled_obj.data() ), compiled_obj.size() );

    devirt::append( compiled_obj, bin );
    std::ofstream( parser.get< std::string >( "out" ), std::ios::binary )
        .write( reinterpret_cast< const char * >( bin.data() ), bin.size() );
}