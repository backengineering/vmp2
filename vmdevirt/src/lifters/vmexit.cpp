#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::vmexit =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder )
    {
        std::stringstream rtn_name;
        llvm::Function *exit_func = nullptr;
        rtn_name << "vmexit_" << std::hex << vinstr.trace_data.vm_handler_rva + rtn->vmp2_file->image_base;

        if ( !( exit_func = rtn->llvm_module->getFunction( rtn_name.str() ) ) )
        {
            auto &ctx = ir_builder->getContext();
            auto i8_ptr_ty = llvm::PointerType::get( llvm::Type::getInt8Ty( ctx ), 0 );
            auto vmexit_func_type = llvm::FunctionType::get( ir_builder->getVoidTy(), { i8_ptr_ty }, false );

            exit_func = llvm::Function::Create( vmexit_func_type, llvm::GlobalValue::ExternalLinkage,
                                                rtn_name.str().c_str(), *rtn->llvm_module );

            auto entry_block = llvm::BasicBlock::Create( ctx, "", exit_func );
            auto vmexit_handler_addr = reinterpret_cast< std::uintptr_t >( rtn->vmp2_file ) +
                                       rtn->vmp2_file->module_offset + vinstr.trace_data.vm_handler_rva;

            zydis_routine_t vmexit_handler;
            vm::util::flatten( vmexit_handler, vmexit_handler_addr );
            vm::util::deobfuscate( vmexit_handler );

            std::string asm_str( "mov rsp, rcx; " );
            ZydisFormatter formatter;
            ZydisFormatterInit( &formatter, ZYDIS_FORMATTER_STYLE_INTEL );

            for ( const auto &instr_data : vmexit_handler )
            {
                if ( instr_data.instr.mnemonic == ZYDIS_MNEMONIC_POP ||
                     instr_data.instr.mnemonic == ZYDIS_MNEMONIC_POPFQ )
                {
                    char buffer[ 256 ];
                    ZydisFormatterFormatInstruction( &formatter, &instr_data.instr, buffer, sizeof( buffer ), 0ull );
                    asm_str.append( buffer ).append( "; " );
                }
            }

            asm_str.append( "ret" );
            auto ib = ir_builder->GetInsertBlock();
            ir_builder->SetInsertPoint( entry_block );

            auto exit_stub = llvm::InlineAsm::get( llvm::FunctionType::get( ir_builder->getVoidTy(), false ), asm_str,
                                                   "", false, false, llvm::InlineAsm::AD_Intel );

            ir_builder->CreateCall( exit_stub );
            ir_builder->CreateRetVoid();
            ir_builder->SetInsertPoint( ib );
        }

        auto &vmp_rtn = rtn->vmp_rtns.back();
        auto &ctx = ir_builder->getContext();
        auto i8_ptr_ty = llvm::PointerType::get( llvm::Type::getInt8Ty( ctx ), 0 );
        auto stack_ptr = ir_builder->CreateLoad( i8_ptr_ty, vmp_rtn->stack );

        ir_builder->CreateCall( exit_func, stack_ptr );
        ir_builder->CreateRet( stack_ptr );
    };
} // namespace vm
