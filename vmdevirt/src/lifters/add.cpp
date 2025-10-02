#include <vm_lifters.hpp>

namespace vm
{
    llvm::Value *lifters_t::add_flags( vm::devirt_t *rtn, std::uint8_t byte_size, llvm::Value *lhs, llvm::Value *rhs )
    {
        auto op_size = llvm::IntegerType::get( *rtn->llvm_ctx, byte_size * 8 );
        std::vector< llvm::Type * > intrinsic_arg_types;
        intrinsic_arg_types.push_back( op_size );
        intrinsic_arg_types.push_back( op_size );

        auto sadd_with_overflow = llvm::Intrinsic::getDeclaration(
            rtn->llvm_module, llvm::Intrinsic::sadd_with_overflow, intrinsic_arg_types );
        auto uadd_with_overflow = llvm::Intrinsic::getDeclaration(
            rtn->llvm_module, llvm::Intrinsic::uadd_with_overflow, intrinsic_arg_types );

        auto u_add = rtn->ir_builder->CreateCall( uadd_with_overflow, { lhs, rhs } );
        auto u_sum = rtn->ir_builder->CreateExtractValue( u_add, { 0 } );
        auto u_of_bit = rtn->ir_builder->CreateExtractValue( u_add, { 1 } );
        auto cf = rtn->ir_builder->CreateZExt( u_of_bit, llvm::IntegerType::get( *rtn->llvm_ctx, 64 ) );

        auto s_add = rtn->ir_builder->CreateCall( sadd_with_overflow, { lhs, rhs } );
        auto s_sum = rtn->ir_builder->CreateExtractValue( s_add, { 0 } );
        auto s_of_bit = rtn->ir_builder->CreateExtractValue( s_add, { 1 } );
        auto of = rtn->ir_builder->CreateZExt( s_of_bit, llvm::IntegerType::get( *rtn->llvm_ctx, 64 ) );

        auto sf = rtn->sf( byte_size, u_sum );
        auto zf = rtn->zf( byte_size, u_sum );
        auto pf = llvm::ConstantInt::get( llvm::IntegerType::get( *rtn->llvm_ctx, 64 ),
                                          0 ); // TODO make clean PF bit computation...

        auto flags_calc =
            rtn->flags( cf, pf, llvm::ConstantInt::get( llvm::IntegerType::get( *rtn->llvm_ctx, 64 ), 0 ), zf, sf, of );

        return flags_calc;
    }

    lifters_t::lifter_callback_t lifters_t::addq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = rtn->pop( 8 );
            auto t3 = ir_builder->CreateAdd( t1, t2 );
            rtn->push( 8, t3 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto flags = lifters_t::add_flags( rtn, 8, t1, t2 );
            ir_builder->CreateStore( flags, vmp_rtn->flags );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::adddw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 4 );
            auto t2 = rtn->pop( 4 );
            auto t3 = ir_builder->CreateAdd( t1, t2 );
            rtn->push( 4, t3 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto flags = lifters_t::add_flags( rtn, 4, t1, t2 );
            ir_builder->CreateStore( flags, vmp_rtn->flags );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::addw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 2 );
            auto t2 = rtn->pop( 2 );
            auto t3 = ir_builder->CreateAdd( t1, t2 );
            rtn->push( 2, t3 );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto flags = lifters_t::add_flags( rtn, 2, t1, t2 );
            ir_builder->CreateStore( flags, vmp_rtn->flags );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

    lifters_t::lifter_callback_t lifters_t::addb =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 2 );
            auto t2 = rtn->pop( 2 );

            auto t3 = ir_builder->CreateIntCast( t1, ir_builder->getInt8Ty(), false );
            auto t4 = ir_builder->CreateIntCast( t2, ir_builder->getInt8Ty(), false );

            auto t5 = ir_builder->CreateAdd( t3, t4 );
            rtn->push( 2, ir_builder->CreateIntCast( t5, ir_builder->getInt16Ty(), false ) );

            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto flags = lifters_t::add_flags( rtn, 1, t3, t4 );
            ir_builder->CreateStore( flags, vmp_rtn->flags );
            rtn->push( 8, rtn->load_value( 8, vmp_rtn->flags ) );
        };

} // namespace vm