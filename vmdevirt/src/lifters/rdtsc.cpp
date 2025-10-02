#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::rdtsc =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            llvm::Function *rdtsc_intrin = nullptr;
            if ( !( rdtsc_intrin = rtn->llvm_module->getFunction( "rdtsc" ) ) )
            {
                rdtsc_intrin = llvm::Function::Create( llvm::FunctionType::get( ir_builder->getVoidTy(), false ),
                                                       llvm::GlobalValue::LinkageTypes::ExternalLinkage, "rdtsc",
                                                       *rtn->llvm_module );

                auto entry_block = llvm::BasicBlock::Create( ir_builder->getContext(), "", rdtsc_intrin );
                auto ib = ir_builder->GetInsertBlock();
                ir_builder->SetInsertPoint( entry_block );

                // TODO: put RDTSC code here...

                ir_builder->CreateRetVoid();
                ir_builder->SetInsertPoint( ib );
            }

            auto &vmp_rtn = rtn->vmp_rtns.back();
            ir_builder->CreateCall( rdtsc_intrin );
        };
}