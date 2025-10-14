#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::pushvspq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder )
    {
        auto &vmp_rtn = rtn->vmp_rtns.back();
        auto &ctx = ir_builder->getContext();
        auto ptr_ty = llvm::PointerType::get( ctx, 0 );
        auto stack = ir_builder->CreateLoad( ptr_ty, vmp_rtn->stack );
        auto stack_ptr = ir_builder->CreatePtrToInt( stack, ir_builder->getInt64Ty() );
        rtn->push( 8, stack_ptr );
    };

    lifters_t::lifter_callback_t lifters_t::pushvspdw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder )
    {
        auto &vmp_rtn = rtn->vmp_rtns.back();
        auto &ctx = ir_builder->getContext();
        auto ptr_ty = llvm::PointerType::get( ctx, 0 );
        auto stack = ir_builder->CreateLoad( ptr_ty, vmp_rtn->stack );
        auto stack_ptr = ir_builder->CreatePtrToInt( stack, ir_builder->getInt32Ty() );
        rtn->push( 4, stack_ptr );
    };
} // namespace vm