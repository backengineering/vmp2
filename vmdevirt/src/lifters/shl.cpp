#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::shlq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = rtn->pop( 2 );
            auto t3 = ir_builder->CreateIntCast( t2, llvm::IntegerType::get( *rtn->llvm_ctx, 64 ), false );
            auto t4 = ir_builder->CreateShl( t1, t3 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            // TODO: update rflags...

            rtn->push( 8, t4 );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::shldw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 4 );
            auto t2 = rtn->pop( 2 );
            auto t3 = ir_builder->CreateIntCast( t2, llvm::IntegerType::get( *rtn->llvm_ctx, 32 ), false );
            auto t4 = ir_builder->CreateShl( t1, t3 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            // TODO: update rflags...

            rtn->push( 4, t4 );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::shlb =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 2 );
            auto t2 = rtn->pop( 2 );
            auto t3 = ir_builder->CreateIntCast( t1, ir_builder->getInt8Ty(), false );
            auto t4 = ir_builder->CreateIntCast( t2, ir_builder->getInt8Ty(), false );
            auto t5 = ir_builder->CreateShl( t3, t4 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            // TODO: update rflags...

            rtn->push( 2, ir_builder->CreateIntCast( t5, ir_builder->getInt16Ty(), false ) );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };
} // namespace vm