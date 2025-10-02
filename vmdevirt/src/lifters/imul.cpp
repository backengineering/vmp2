#include <vm_lifters.hpp>

// https://lists.llvm.org/pipermail/llvm-dev/2014-July/074685.html
// credit to James Courtier-Dutton for asking this question in 2014...
namespace vm
{
    lifters_t::lifter_callback_t lifters_t::imulq =
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

    lifters_t::lifter_callback_t lifters_t::imuldw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 4 );
            auto t2 = rtn->pop( 4 );

            auto t3 = ir_builder->CreateIntCast( t1, ir_builder->getInt64Ty(), false );
            auto t4 = ir_builder->CreateIntCast( t2, ir_builder->getInt64Ty(), false );

            auto t5 = ir_builder->CreateMul( t3, t4 );
            auto t6 = ir_builder->CreateAShr( t5, llvm::APInt( 64, 32 ) );
            auto t7 = ir_builder->CreateAnd( t5, 0xFFFFFFFF00000000 );

            rtn->push( 8, t6 );
            rtn->push( 8, t7 );

            // TODO: compute flags for IMULQ
            auto &vmp_rtn = rtn->vmp_rtns.back();
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };
} // namespace vm