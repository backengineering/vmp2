#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::divq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = rtn->pop( 8 );
            auto t3 = rtn->pop( 8 );
            ir_builder->CreateUDiv( t2, t3 );
            rtn->push( 8, t1 );
            rtn->push( 8, t2 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            // TODO: compute flags...
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::divdw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 4 );
            auto t2 = rtn->pop( 4 );
            auto t3 = rtn->pop( 4 );
            ir_builder->CreateUDiv( t2, t3 );
            rtn->push( 4, t1 );
            rtn->push( 4, t2 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            // TODO: compute flags...
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };
} // namespace vm