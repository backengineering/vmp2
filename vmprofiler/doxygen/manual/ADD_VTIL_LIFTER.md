# Adding VMProtect 2 IL to VTIL Lifters

This will disclose how to create a VTIL lifter for VMProfiler v1.8. The instructions may change in later versions of VMProfiler.

# Example - Existing VTIL Lifter For LCONSTQ

Understand that LCONSTQ loads an eight byte value onto the stack. Thus the usage of `vtil::operand` to create a 64 bit value.

#### Step 1, Declare Lifter

```
vm::lifters::lifter_t lconstq = {
    // push imm<N>
    vm::handler::LCONSTQ,
    []( vtil::basic_block *blk, vm::instrs::virt_instr_t *vinstr, vmp2::v3::code_block_t *code_blk ) {
        blk->push( vtil::operand( vinstr->operand.imm.u, 64 ) );
    } };
```

#### Step 2, Declare Extern In vmlifters.hpp

You can see this exact line of code [here](https://githacks.org/vmp2/vmprofiler/-/blob/8baefa1e2148111712d640ee9cb7c0b7ac329521/include/vmlifters.hpp#L22).

```cpp
extern vm::lifters::lifter_t lconstq;
```

#### Step 3, Add Lifter To vm::lifters::all

```cpp
inline std::vector< vm::lifters::lifter_t * > all = {
    // lreg lifters...
    &lregq, &lregdw,

    // add lifters...
    &addq, &adddw, &addw,

    // sreg lifters...
    &sregq, &sregdw, &sregw,

    // lconst lifters...
    &lconstq, &lconstdw, &lconstw, &lconstbzxw, &lconstbsxdw, &lconstbsxq, &lconstdwsxq, &lconstwsxq, &lconstwsxdw,

    // nand lifters...
    &nandq, &nanddw, &nandw,

    // read lifters....
    &readq, &readdw, &readw,

    // shr lifters...
    &shrq, &shrw,

    // pushvsp lifter...
    &pushvsp,

    // jmp lifter...
    &jmp,

    // lflags lifter...
    &lrflags,

    // lvsp lifter...
    &lvsp,

    // vmexit lifter...
    &vmexit };
```