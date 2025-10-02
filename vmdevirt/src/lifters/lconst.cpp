#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::lconstq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            rtn->push( 8, llvm::ConstantInt::get( ir_builder->getInt64Ty(), vinstr.operand.imm.u ) );
        };

    lifters_t::lifter_callback_t lifters_t::lconstdwsxq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            rtn->push( 8, llvm::ConstantInt::get( ir_builder->getInt64Ty(), vinstr.operand.imm.u ) );
        };

    lifters_t::lifter_callback_t lifters_t::lconstwsxq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            rtn->push( 8, llvm::ConstantInt::get( ir_builder->getInt64Ty(), vinstr.operand.imm.u ) );
        };

    lifters_t::lifter_callback_t lifters_t::lconstbsxq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            rtn->push( 8, llvm::ConstantInt::get( ir_builder->getInt64Ty(), vinstr.operand.imm.u ) );
        };

    lifters_t::lifter_callback_t lifters_t::lconstdw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            rtn->push( 4, llvm::ConstantInt::get( ir_builder->getInt32Ty(), vinstr.operand.imm.u ) );
        };

    lifters_t::lifter_callback_t lifters_t::lconstw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            rtn->push( 2, llvm::ConstantInt::get( ir_builder->getInt16Ty(), vinstr.operand.imm.u ) );
        };

    lifters_t::lifter_callback_t lifters_t::lconstwsxdw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            rtn->push( 4, llvm::ConstantInt::get( ir_builder->getInt32Ty(), vinstr.operand.imm.u ) );
        };

    lifters_t::lifter_callback_t lifters_t::lconstbsxdw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            rtn->push( 4, llvm::ConstantInt::get( ir_builder->getInt32Ty(), vinstr.operand.imm.u ) );
        };

    lifters_t::lifter_callback_t lifters_t::lconstbzxw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            rtn->push( 2, llvm::ConstantInt::get( ir_builder->getInt16Ty(), vinstr.operand.imm.u ) );
        };
} // namespace vm