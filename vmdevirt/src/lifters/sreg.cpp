#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::sregq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 8 );
            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto vreg = vmp_rtn->virtual_registers[ vinstr.operand.imm.u ? vinstr.operand.imm.u / 8 : 0 ];
            ir_builder->CreateStore( t1, vreg );
        };

    lifters_t::lifter_callback_t lifters_t::sregdw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 4 );
            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto vreg = vmp_rtn->virtual_registers[ vinstr.operand.imm.u ? vinstr.operand.imm.u / 8 : 0 ];
            ir_builder->CreateStore( ir_builder->CreateIntCast( t1, ir_builder->getInt64Ty(), false ), vreg );
        };

    lifters_t::lifter_callback_t lifters_t::sregw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 2 );
            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto vreg = vmp_rtn->virtual_registers[ vinstr.operand.imm.u ? vinstr.operand.imm.u / 8 : 0 ];
            ir_builder->CreateStore(
                ir_builder->CreateIntCast( t1, ir_builder->getInt16Ty(), false ),
                ir_builder->CreatePointerCast( vreg, llvm::PointerType::get( ir_builder->getInt16Ty(), 0ull ) ) );
        };

    lifters_t::lifter_callback_t lifters_t::sregb =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto t1 = rtn->pop( 2 );
            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto vreg = vmp_rtn->virtual_registers[ vinstr.operand.imm.u ? vinstr.operand.imm.u / 8 : 0 ];
            ir_builder->CreateStore(
                ir_builder->CreateIntCast( t1, ir_builder->getInt8Ty(), false ),
                ir_builder->CreatePointerCast( vreg, llvm::PointerType::get( ir_builder->getInt8Ty(), 0ull ) ) );
        };
} // namespace vm