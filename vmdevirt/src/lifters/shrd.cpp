#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::shrdq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = rtn->pop( 8 );
            auto t3 = rtn->pop( 2 );

            // TODO: this is wrong - replace with more logic!
            auto t4 = ir_builder->CreateLShr( t1, ir_builder->CreateIntCast( t3, ir_builder->getInt64Ty(), false ) );

            rtn->push( 8, t4 );
            auto &vmp_rtn = rtn->vmp_rtns.back();

            // TODO: update rflags...
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::shrddw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 4 );
            auto t2 = rtn->pop( 4 );
            auto t3 = rtn->pop( 2 );

            // TODO: this is wrong - replace with more logic!
            auto t4 = ir_builder->CreateLShr( t1, ir_builder->CreateIntCast( t3, ir_builder->getInt32Ty(), false ) );

            rtn->push( 4, t4 );
            auto &vmp_rtn = rtn->vmp_rtns.back();

            // TODO: update rflags...
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };
} // namespace vm