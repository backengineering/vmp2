#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::lregq =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto vreg = vmp_rtn->virtual_registers[ vinstr.operand.imm.u ? vinstr.operand.imm.u / 8 : 0 ];
            rtn->push( 8, rtn->load_value( 8, vreg ) );
        };

    lifters_t::lifter_callback_t lifters_t::lregdw =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            auto &vmp_rtn = rtn->vmp_rtns.back();
            auto vreg = vmp_rtn->virtual_registers[ vinstr.operand.imm.u ? vinstr.operand.imm.u / 8 : 0 ];
            rtn->push( 4, rtn->load_value( 4, vreg ) );
        };
} // namespace vm