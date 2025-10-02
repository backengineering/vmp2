#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::lflagsq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto flags = rtn->pop( 8 );
            ir_builder->CreateStore( flags, vmp_rtn->flags );
        };
}