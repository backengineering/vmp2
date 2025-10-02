# Adding VMProtect 2 Virtual Instruction Profiles

This page contains the steps needed to add additional VMProtect 2 virtual instruction profiles to VMProfiler. Understand that these instructions are for version 1.8 and may be different in later versions. 

# Example - Existing Profile

Consider the `ADDQ` profile which is displayed below which can be found inside of `add.cpp`. Notice that not every single instruction of the vm handler needs to be declared inside of the zydis lambda vector, however you will be required to be as explicit as it requires for each vm handler to have a unique signature.

#### Step 1, Define The Profile

```cpp
vm::handler::profile_t addq = {
    // ADD [RBP+8], RAX
    // PUSHFQ
    // POP [RBP]
    "ADDQ",
    ADDQ,
    NULL,
    { { // ADD [RBP+8], RAX
        []( const zydis_decoded_instr_t &instr ) -> bool {
            return instr.mnemonic == ZYDIS_MNEMONIC_ADD && instr.operands[ 0 ].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                   instr.operands[ 0 ].mem.base == ZYDIS_REGISTER_RBP &&
                   instr.operands[ 0 ].mem.disp.value == 0x8 &&
                   instr.operands[ 1 ].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                   instr.operands[ 1 ].reg.value == ZYDIS_REGISTER_RAX;
        },
        // PUSHFQ
        []( const zydis_decoded_instr_t &instr ) -> bool { return instr.mnemonic == ZYDIS_MNEMONIC_PUSHFQ; },
        // POP [RBP]
        []( const zydis_decoded_instr_t &instr ) -> bool {
            return instr.mnemonic == ZYDIS_MNEMONIC_POP && instr.operands[ 0 ].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                   instr.operands[ 0 ].mem.base == ZYDIS_REGISTER_RBP;
        } } } };
```

Inside of `vmprofiles.hpp` you can see a list of these profiles marked as `extern`.

#### Step 2, Declare It Extern Inside vmprofiles.hpp

```cpp
namespace profile
{
    extern vm::handler::profile_t sregq;
    extern vm::handler::profile_t sregdw;
    extern vm::handler::profile_t sregw;

    extern vm::handler::profile_t lregq;
    extern vm::handler::profile_t lregdw;

    extern vm::handler::profile_t lconstq;
    extern vm::handler::profile_t lconstdw;
    extern vm::handler::profile_t lconstw;

    extern vm::handler::profile_t lconstbzxw;
    extern vm::handler::profile_t lconstbsxdw;
    extern vm::handler::profile_t lconstbsxq;
    extern vm::handler::profile_t lconstdwsxq;
    extern vm::handler::profile_t lconstwsxq;
    extern vm::handler::profile_t lconstwsxdw;

    extern vm::handler::profile_t addq; // as you can see a reference to addq is declared here...
    ...
}
```

Lastly the `addq` variable is added to a vector of `vm::handler::profile_t*`'s.

#### Step 3, Add The Variable To vm::handler::profile::all

```cpp
inline std::vector< vm::handler::profile_t * > all = {
    &sregq,      &sregdw,      &sregw,      &lregq,       &lregdw,     &lconstq,
    &lconstbzxw, &lconstbsxdw, &lconstbsxq, &lconstdwsxq, &lconstwsxq, &lconstwsxdw,
    &lconstdw,   &lconstw,     &addq,       &adddw,       &addw,       &lvsp,

    &shlq,       &shldw,       &writeq,     &writedw,     &writeb,     &nandq,
    &nanddw,     &nandw,       &nandb,

    &shlddw,

    &shrq,       &shrw,        &readq,      &readdw,      &mulq,       &pushvsp,
    &divq,       &jmp,         &lrflags,    &vmexit,      &call };
```