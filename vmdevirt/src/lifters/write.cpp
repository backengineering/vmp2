#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::writeq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = rtn->pop( 8 );
            auto t3 = ir_builder->CreateIntToPtr( t1, llvm::PointerType::get( ir_builder->getInt64Ty(), 0ull ) );
            ir_builder->CreateStore( t2, t3 );
        };

    lifters_t::lifter_callback_t lifters_t::writedw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = rtn->pop( 4 );
            auto t3 = ir_builder->CreateIntToPtr( t1, llvm::PointerType::get( ir_builder->getInt32Ty(), 0ull ) );
            ir_builder->CreateStore( t2, t3 );
        };

    lifters_t::lifter_callback_t lifters_t::writew =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = rtn->pop( 2 );
            auto t3 = ir_builder->CreateIntToPtr( t1, llvm::PointerType::get( ir_builder->getInt16Ty(), 0ull ) );
            ir_builder->CreateStore( t2, t3 );
        };

    lifters_t::lifter_callback_t lifters_t::writeb =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = rtn->pop( 2 );
            auto t3 = ir_builder->CreateIntToPtr( t1, llvm::PointerType::get( ir_builder->getInt8Ty(), 0ull ) );
            auto t4 = ir_builder->CreateIntCast( t2, ir_builder->getInt8Ty(), false );
            ir_builder->CreateStore( t4, t3 );
        };
} // namespace vm