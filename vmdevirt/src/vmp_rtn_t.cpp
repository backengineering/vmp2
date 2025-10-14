#include <sstream>
#include <vmp_rtn_t.hpp>

namespace vm
{
    vmp_rtn_t::vmp_rtn_t( std::uintptr_t rtn_begin, std::vector< vm::instrs::code_block_t > vmp2_code_blocks,
                          std::shared_ptr< llvm::IRBuilder<> > &ir_builder, llvm::Module *llvm_module,
                          vmp2::v4::file_header *vmp2_file )
        : ir_builder( ir_builder ), vmp2_code_blocks( vmp2_code_blocks ), rtn_begin( rtn_begin ),
          llvm_module( llvm_module ), vmp2_file( vmp2_file )
    {
        create_routine();
        // create virtual registers in the first code block...
        ir_builder->SetInsertPoint( llvm_code_blocks[ 0 ].second );
        flags = ir_builder->CreateAlloca( ir_builder->getInt64Ty(), nullptr, "flags" );
        stack = ir_builder->CreateAlloca( llvm::PointerType::get( ir_builder->getInt8Ty(), 0ull ), nullptr, "sp" );
        ir_builder->CreateStore( llvm_fptr->getArg( 0 ), stack );
        create_virtual_registers();
    }

    void vmp_rtn_t::create_virtual_registers( void )
    {
        for ( auto idx = 0u; idx < 24; ++idx )
            // allocate virtual register space...
            virtual_registers.push_back(
                ir_builder->CreateAlloca( llvm::IntegerType::get( ir_builder->getContext(), 64 ), nullptr,
                                          ( std::string( "vreg" ) + std::to_string( idx ) ).c_str() ) );
    }

void vmp_rtn_t::create_routine()
    {
        auto &ctx = ir_builder->getContext();

        // Define i8* type (since getInt8PtrTy is now on Type, not PointerType)
        auto i8_ptr_ty = llvm::PointerType::get( llvm::Type::getInt8Ty( ctx ), 0 );

        // Function type: i8* (i8*)
        auto func_ty = llvm::FunctionType::get( i8_ptr_ty, { i8_ptr_ty }, false );

        std::stringstream rtn_name;
        rtn_name << "rtn_" << std::hex << rtn_begin;

        llvm_fptr = llvm::Function::Create( func_ty, llvm::GlobalValue::ExternalLinkage, rtn_name.str(), *llvm_module );

        // Create basic blocks for each VMProtect code block
        for ( const auto &vmp2_code_block : vmp2_code_blocks )
        {
            std::stringstream blk_name;
            blk_name << "blk_" << std::hex << vmp2_code_block.vip_begin;

            llvm_code_blocks.push_back(
                { vmp2_code_block.vip_begin, llvm::BasicBlock::Create( ctx, blk_name.str(), llvm_fptr ) } );
        }

        // Load and deobfuscate the VM entry routine
        zydis_routine_t vm_enter;
        auto vm_enter_addr = reinterpret_cast< std::uintptr_t >( vmp2_file ) + vmp2_file->module_offset +
                             ( rtn_begin - vmp2_file->image_base );

        vm::util::flatten( vm_enter, vm_enter_addr );
        vm::util::deobfuscate( vm_enter );

        std::string asm_str;
        ZydisFormatter formatter;
        ZydisFormatterInit( &formatter, ZYDIS_FORMATTER_STYLE_INTEL );

        for ( const auto &instr_data : vm_enter )
        {
            if ( instr_data.instr.mnemonic == ZYDIS_MNEMONIC_PUSH )
            {
                if ( instr_data.instr.operands[ 0 ].type == ZYDIS_OPERAND_TYPE_MEMORY )
                {
                    asm_str.append( "mov rax, 0x1000000000000000; push rax; mov rax, 0x1000000000000000; push rax; " );
                    break;
                }

                char buffer[ 256 ];
                ZydisFormatterFormatInstruction( &formatter, &instr_data.instr, buffer, sizeof( buffer ), 0ull );
                asm_str.append( buffer ).append( "; " );
            }
            else if ( instr_data.instr.mnemonic == ZYDIS_MNEMONIC_PUSHFQ )
            {
                asm_str.append( "pushfq; " );
            }
        }

        // Build the entry stub function
        rtn_name.str( "" );
        rtn_name << "vmenter_" << std::hex << rtn_begin;

        asm_str.append( "mov rcx, rsp; sub rsp, 0xA00; int 3; int 3; int 3; int 3; int 3;" );

        auto entry_func = llvm::Function::Create( llvm::FunctionType::get( ir_builder->getVoidTy(), false ),
                                                  llvm::GlobalValue::ExternalLinkage, rtn_name.str(), *llvm_module );

        auto entry_block = llvm::BasicBlock::Create( ctx, "", entry_func );
        ir_builder->SetInsertPoint( entry_block );

        auto entry_stub = llvm::InlineAsm::get( llvm::FunctionType::get( ir_builder->getVoidTy(), false ), asm_str, "",
                                                false, false, llvm::InlineAsm::AD_Intel );

        ir_builder->CreateCall( entry_stub );
        ir_builder->CreateRetVoid();
    }
} // namespace vm