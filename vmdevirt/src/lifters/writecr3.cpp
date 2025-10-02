#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::writecr3 =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            llvm::Function *writecr3_intrin = nullptr;
            if ( !( writecr3_intrin = rtn->llvm_module->getFunction( "writecr3" ) ) )
            {
                writecr3_intrin = llvm::Function::Create(
                    llvm::FunctionType::get( ir_builder->getVoidTy(), { ir_builder->getInt64Ty() }, false ),
                    llvm::GlobalValue::LinkageTypes::ExternalLinkage, "writecr3", *rtn->llvm_module );

                auto entry_block = llvm::BasicBlock::Create( ir_builder->getContext(), "", writecr3_intrin );
                auto ib = ir_builder->GetInsertBlock();
                ir_builder->SetInsertPoint( entry_block );

                std::string asm_str( "mov cr3, rcx; ret" );
                auto intrin = llvm::InlineAsm::get(
                    llvm::FunctionType::get( ir_builder->getVoidTy(), false ), asm_str,
                    "", false, false, llvm::InlineAsm::AD_Intel );

                ir_builder->CreateCall( intrin );
                ir_builder->CreateRetVoid();
                ir_builder->SetInsertPoint( ib );
            }
            auto t1 = rtn->pop( 8 );
            ir_builder->CreateCall( writecr3_intrin, { t1 } );
        };
}