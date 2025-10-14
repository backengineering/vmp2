#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::popvsp =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder )
    {
        auto &vmp_rtn = rtn->vmp_rtns.back();
        auto &ctx = ir_builder->getContext();

        auto ptr_ty = llvm::PointerType::get( ctx, 0 );
        auto stack = ir_builder->CreateLoad( ptr_ty, vmp_rtn->stack );

        auto i8_ptr_ty = llvm::PointerType::get( llvm::Type::getInt8Ty( ctx ), 0 );
        auto i8_ptr_ptr_ty = llvm::PointerType::get( i8_ptr_ty, 0 );
        auto stack_ptr_ptr = ir_builder->CreatePointerCast( stack, i8_ptr_ptr_ty );

        auto stack_ptr = ir_builder->CreateLoad( ptr_ty, stack_ptr_ptr );
        ir_builder->CreateStore( stack_ptr, vmp_rtn->stack );
    };
} // namespace vm