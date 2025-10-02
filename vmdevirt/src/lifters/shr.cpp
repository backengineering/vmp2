#include <vm_lifters.hpp>

namespace vm
{
    llvm::Value *lifters_t::shr_flags( vm::devirt_t *rtn, std::uint8_t byte_size, llvm::Value *lhs, llvm::Value *rhs,
                                       llvm::Value *result )
    {
        auto op_size = llvm::IntegerType::get( *rtn->llvm_ctx, byte_size * 8 );
        auto msb = rtn->ir_builder->CreateLShr( lhs, ( byte_size * 8 ) - 1 );

        auto cf = rtn->ir_builder->CreateZExt( msb, llvm::IntegerType::get( *rtn->llvm_ctx, 64 ) );
        auto of = rtn->sf( byte_size, lhs );
        auto sf = rtn->sf( byte_size, result );
        auto zf = rtn->zf( byte_size, result );
        auto pf = llvm::ConstantInt::get( llvm::IntegerType::get( *rtn->llvm_ctx, 64 ), 0 );

        return rtn->flags( cf, pf, llvm::ConstantInt::get( llvm::IntegerType::get( *rtn->llvm_ctx, 64 ), 0 ), zf, sf,
                           of );
    }

    lifters_t::lifter_callback_t lifters_t::shrq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = rtn->pop( 2 );
            auto t3 = ir_builder->CreateIntCast( t2, llvm::IntegerType::get( *rtn->llvm_ctx, 64 ), false );
            auto t4 = ir_builder->CreateLShr( t1, t3 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto flags = lifters_t::shr_flags( rtn, 8, t1, t3, t4 );
            ir_builder->CreateStore( flags, vmp_rtn->flags );
            rtn->push( 8, t4 );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::shrdw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 4 );
            auto t2 = rtn->pop( 2 );
            auto t3 = ir_builder->CreateIntCast( t2, llvm::IntegerType::get( *rtn->llvm_ctx, 32 ), false );
            auto t4 = ir_builder->CreateLShr( t1, t3 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto flags = lifters_t::shr_flags( rtn, 4, t1, t3, t4 );
            ir_builder->CreateStore( flags, vmp_rtn->flags );
            rtn->push( 4, t4 );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::shrw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 2 );
            auto t2 = rtn->pop( 2 );
            auto t3 = ir_builder->CreateLShr( t1, t2 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto flags = lifters_t::shr_flags( rtn, 2, t1, t2, t3 );
            ir_builder->CreateStore( flags, vmp_rtn->flags );
            rtn->push( 2, t3 );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::shrb =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 2 );
            auto t2 = rtn->pop( 2 );
            auto t3 = ir_builder->CreateIntCast( t1, ir_builder->getInt8Ty(), false );
            auto t4 = ir_builder->CreateIntCast( t2, ir_builder->getInt8Ty(), false );
            auto t5 = ir_builder->CreateLShr( t3, t4 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto flags = lifters_t::shr_flags( rtn, 1, t3, t4, t5 );
            ir_builder->CreateStore( flags, vmp_rtn->flags );
            rtn->push( 2, ir_builder->CreateIntCast( t5, ir_builder->getInt16Ty(), false ) );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };
} // namespace vm