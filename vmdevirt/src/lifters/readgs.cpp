#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::readgsq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            llvm::Function *readgs_intrin = nullptr;
            if ( !( readgs_intrin = rtn->llvm_module->getFunction( "readgs" ) ) )
            {
                readgs_intrin = llvm::Function::Create(
                    llvm::FunctionType::get( ir_builder->getInt64Ty(), { ir_builder->getInt64Ty() }, false ),
                    llvm::GlobalValue::LinkageTypes::ExternalLinkage, "readgs", *rtn->llvm_module );

                auto entry_block = llvm::BasicBlock::Create( ir_builder->getContext(), "", readgs_intrin );
                auto ib = ir_builder->GetInsertBlock();
                ir_builder->SetInsertPoint( entry_block );

                std::string asm_str( "mov rax, gs:[rcx]; ret" );
                auto intrin = llvm::InlineAsm::get( llvm::FunctionType::get( ir_builder->getVoidTy(), false ), asm_str,
                                                    "", false, false, llvm::InlineAsm::AD_Intel );

                ir_builder->CreateCall( intrin );
                ir_builder->CreateRet( llvm::ConstantInt::get( *rtn->llvm_ctx, llvm::APInt( 64, 0 ) ) );
                ir_builder->SetInsertPoint( ib );
            }

            auto t1 = rtn->pop( 8 );
            auto t2 = ir_builder->CreateCall( readgs_intrin, { t1 } );
            rtn->push( 8, t2 );
        };
} // namespace vm