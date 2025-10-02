#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::mulq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = rtn->pop( 8 );

            // TODO: this is wrong... still need to do some more research into this...
            auto t3 = ir_builder->CreateMul( t1, t2 );
            auto t4 = ir_builder->CreateAShr( t3, llvm::APInt( 64, 32 ) );
            auto t5 = ir_builder->CreateAnd( t3, 0xFFFFFFFF00000000 );
            rtn->push( 8, t4 );
            rtn->push( 8, t5 );

            // TODO: compute flags for IMULQ
            auto &vmp_rtn = rtn->vmp_rtns.back();
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::muldw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 4 );
            auto t2 = rtn->pop( 4 );

            // TODO: this is wrong... still need to do some more research into this...
            auto t3 = ir_builder->CreateMul( t1, t2 );
            auto t4 = ir_builder->CreateAShr( t3, llvm::APInt( 32, 16 ) );
            auto t5 = ir_builder->CreateAnd( t3, 0xFFFF0000 );
            rtn->push( 4, t4 );
            rtn->push( 4, t5 );

            // TODO: compute flags for IMULQ
            auto &vmp_rtn = rtn->vmp_rtns.back();
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };
}