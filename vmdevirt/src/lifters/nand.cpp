#include <vm_lifters.hpp>

namespace vm
{
    llvm::Value *lifters_t::and_flags( vm::devirt_t *rtn, std::uint8_t byte_size, llvm::Value *result )
    {
        auto cf = llvm::ConstantInt::get( llvm::IntegerType::get( *rtn->llvm_ctx, 64 ), 0 );
        auto of = llvm::ConstantInt::get( llvm::IntegerType::get( *rtn->llvm_ctx, 64 ), 0 );

        auto sf = rtn->sf( byte_size, result );
        auto zf = rtn->zf( byte_size, result );
        auto pf = llvm::ConstantInt::get( llvm::IntegerType::get( *rtn->llvm_ctx, 64 ), 0 );

        return rtn->flags( cf, pf, llvm::ConstantInt::get( llvm::IntegerType::get( *rtn->llvm_ctx, 64 ), 0 ), zf, sf,
                           of );
    }

    lifters_t::lifter_callback_t lifters_t::nandq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = rtn->pop( 8 );

            auto t1_not = ir_builder->CreateNot( t1 );
            auto t2_not = ir_builder->CreateNot( t2 );

            auto t3 = ir_builder->CreateAnd( { t1_not, t2_not } );
            rtn->push( 8, t3 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto flags = and_flags( rtn, 8, t3 );
            ir_builder->CreateStore( flags, vmp_rtn->flags );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::nanddw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 4 );
            auto t2 = rtn->pop( 4 );

            auto t1_not = ir_builder->CreateNot( t1 );
            auto t2_not = ir_builder->CreateNot( t2 );

            auto t3 = ir_builder->CreateAnd( { t1_not, t2_not } );
            rtn->push( 4, t3 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto flags = and_flags( rtn, 4, t3 );
            ir_builder->CreateStore( flags, vmp_rtn->flags );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::nandw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 2 );
            auto t2 = rtn->pop( 2 );

            auto t1_not = ir_builder->CreateNot( t1 );
            auto t2_not = ir_builder->CreateNot( t2 );

            auto t3 = ir_builder->CreateAnd( { t1_not, t2_not } );
            rtn->push( 2, t3 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto flags = and_flags( rtn, 2, t3 );
            ir_builder->CreateStore( flags, vmp_rtn->flags );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::nandb =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 2 );
            auto t2 = rtn->pop( 2 );

            auto t1_b = ir_builder->CreateIntCast( t1, ir_builder->getInt8Ty(), false );
            auto t2_b = ir_builder->CreateIntCast( t2, ir_builder->getInt8Ty(), false );

            auto t1_not = ir_builder->CreateNot( t1_b );
            auto t2_not = ir_builder->CreateNot( t2_b );

            auto t3 = ir_builder->CreateAnd( { t1_not, t2_not } );
            auto t3_w = ir_builder->CreateIntCast( t3, ir_builder->getInt16Ty(), false );

            rtn->push( 2, t3_w );
            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto flags = and_flags( rtn, 1, t3 );
            ir_builder->CreateStore( flags, vmp_rtn->flags );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

} // namespace vm