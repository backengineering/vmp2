#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::readcr8 =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            llvm::Function *readcr8_intrin = nullptr;
            if ( !( readcr8_intrin = rtn->llvm_module->getFunction( "readcr8" ) ) )
            {
                readcr8_intrin = llvm::Function::Create( llvm::FunctionType::get( ir_builder->getInt64Ty(), false ),
                                                         llvm::GlobalValue::LinkageTypes::ExternalLinkage, "readcr8",
                                                         *rtn->llvm_module );

                auto entry_block = llvm::BasicBlock::Create( ir_builder->getContext(), "", readcr8_intrin );
                auto ib = ir_builder->GetInsertBlock();
                ir_builder->SetInsertPoint( entry_block );

                std::string asm_str( "mov rax, cr8; ret" );
                auto intrin = llvm::InlineAsm::get( llvm::FunctionType::get( ir_builder->getVoidTy(), false ), asm_str,
                                                    "", false, false, llvm::InlineAsm::AD_Intel );

                ir_builder->CreateCall( intrin );
                ir_builder->CreateRet( llvm::ConstantInt::get( *rtn->llvm_ctx, llvm::APInt( 64, 0 ) ) );
                ir_builder->SetInsertPoint( ib );
            }
            auto t1 = ir_builder->CreateCall( readcr8_intrin );
            rtn->push( 8, t1 );
        };
}