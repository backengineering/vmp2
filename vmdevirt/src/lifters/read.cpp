#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::readq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = ir_builder->CreateIntToPtr( t1, llvm::PointerType::get( ir_builder->getInt64Ty(), 0ull ) );
            auto t3 = ir_builder->CreateLoad( ir_builder->getInt64Ty(), t2 );
            rtn->push( 8, t3 );
        };

    lifters_t::lifter_callback_t lifters_t::readdw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = ir_builder->CreateIntToPtr( t1, llvm::PointerType::get( ir_builder->getInt32Ty(), 0ull ) );
            auto t3 = ir_builder->CreateLoad( ir_builder->getInt32Ty(), t2 );
            rtn->push( 4, t3 );
        };

    lifters_t::lifter_callback_t lifters_t::readw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = ir_builder->CreateIntToPtr( t1, llvm::PointerType::get( ir_builder->getInt16Ty(), 0ull ) );
            auto t3 = ir_builder->CreateLoad( ir_builder->getInt16Ty(), t2 );
            rtn->push( 2, t3 );
        };

    lifters_t::lifter_callback_t lifters_t::readb =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto t2 = ir_builder->CreateIntToPtr( t1, llvm::PointerType::get( ir_builder->getInt8Ty(), 0ull ) );
            auto t3 = ir_builder->CreateLoad( ir_builder->getInt8Ty(), t2 );
            auto t4 = ir_builder->CreateIntCast( t3, ir_builder->getInt16Ty(), false );
            rtn->push( 2, t4 );
        };
} // namespace vm