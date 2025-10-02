#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::popvsp =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto stack = ir_builder->CreateLoad( vmp_rtn->stack );
            auto stack_ptr_ptr = ir_builder->CreatePointerCast(
                stack, llvm::PointerType::get( llvm::PointerType::get( ir_builder->getInt8Ty(), 0ull ), 0ull ) );

            auto stack_ptr = ir_builder->CreateLoad( stack_ptr_ptr );
            ir_builder->CreateStore( stack_ptr, vmp_rtn->stack );
        };
}