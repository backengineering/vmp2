<div align="center">
    <div>
        <img src="https://githacks.org/uploads/-/system/project/avatar/371/VMProtect.png"/>
    </div>
</div>

# VMProfiler CLI - VMProtect 2 Virtual Machine Information Displayer

vmprofiler-cli is a CLI program which displays all details of a specified VMProtect 2 virtual machine. This information includes virtual instruction pointer advancment direction, all vm handlers, virtual instruction rva decrypt instructions, vm handler table entry decrypt instruction, and much more.

# Build Instructions (Windows)

### Requirements

* Visual Studios 2019 *Fully Updated*
* CMake
* A good attitude

Clone the repository and all submodules using `git clone --recursive https://githacks.org/vmp2/vmprofiler-cli.git`. Then execute the following commands:

```
cd vmprofiler-cli
cmake -B build
```

Now you can open `vmprofiler-cli.sln`, select "Release" and "x64" on the top of the visual studios window. Building should take a few minutes.

# Build Instructions (Linux)

### Requirements

* C++ 2020 STL (libstdc++-10-dev)
* Clang 10
* CMake 3.X

```
sudo apt install clang-10 libstdc++-10-dev cmake
```

### Download & Build

```
git clone --recursive https://githacks.org/vmp2/vmprofiler-cli.git
cd vmprofiler-cli

export CC=/usr/bin/clang-10
export CXX=/usr/bin/clang++-10

cmake -B build && cd build
make
```

# usage

```
Usage: vmprofiler-cli [options...]
Options:
    --bin, --vmpbin        unpacked binary protected with VMProtect 2
    --vmentry, --entry     rva to push prior to a vm_entry
    --showhandlers         show all vm handlers...
    --showhandler          show a specific vm handler given its index...
    --indexes              displays vm handler table indexes for a given vm handler name such as 'READQ', or 'WRITEQ'...
    -h, --help             Shows this page
```

# output - information example

```
vmprofiler-cli.exe --vmpbin vmptest.vmp.exe --vmentry 0x1000
> vm entry start = 0x00007FF61C8B1000
> flattened vm entry...
> deobfuscated vm entry...
==================================================================================
> 0x00007FF61C8B822C push 0xFFFFFFFF890001FA
> 0x00007FF61C8B7FC9 push 0x45D3BF1F
> 0x00007FF61C8B48E4 push r13
> 0x00007FF61C8B4690 push rsi
> 0x00007FF61C8B4E53 push r14
> 0x00007FF61C8B74FB push rcx
> 0x00007FF61C8B607C push rsp
> 0x00007FF61C8B4926 pushfq
> 0x00007FF61C8B4DC2 push rbp
> 0x00007FF61C8B5C8C push r12
> 0x00007FF61C8B52AC push r10
> 0x00007FF61C8B51A5 push r9
> 0x00007FF61C8B5189 push rdx
> 0x00007FF61C8B7D5F push r8
> 0x00007FF61C8B4505 push rdi
> 0x00007FF61C8B4745 push r11
> 0x00007FF61C8B478B push rax
> 0x00007FF61C8B7A53 push rbx
> 0x00007FF61C8B500D push r15
> 0x00007FF61C8B6030 push [0x00007FF61C8B7912]
> 0x00007FF61C8B593A mov rax, 0x7FF4DC8B0000
> 0x00007FF61C8B5955 mov r13, rax
> 0x00007FF61C8B595F test dl, al
> 0x00007FF61C8B5965 push rax
> 0x00007FF61C8B5969 btr si, bx
> 0x00007FF61C8B596F mov esi, [rsp+0xA0]
> 0x00007FF61C8B5979 not esi
> 0x00007FF61C8B5985 neg esi
> 0x00007FF61C8B598D ror esi, 0x1A
> 0x00007FF61C8B599E mov rbp, rsp
> 0x00007FF61C8B59A8 sub rsp, 0x140
> 0x00007FF61C8B59B5 and rsp, 0xFFFFFFFFFFFFFFF0
> 0x00007FF61C8B59BE inc ax
> 0x00007FF61C8B59C1 mov rdi, rsp
> 0x00007FF61C8B59C7 bsr r12, rax
> 0x00007FF61C8B59CB lea r12, [0x00007FF61C8B6473]
> 0x00007FF61C8B59DF mov rax, 0x100000000
> 0x00007FF61C8B59EC add rsi, rax
> 0x00007FF61C8B59F3 mov rbx, rsi
> 0x00007FF61C8B59FA add rsi, [rbp]
> 0x00007FF61C8B5A03 rcr dl, cl
> 0x00007FF61C8B5A05 mov al, [rsi]
> 0x00007FF61C8B5A0A xor al, bl
> 0x00007FF61C8B5A11 neg al
> 0x00007FF61C8B5A19 rol al, 0x05
> 0x00007FF61C8B5A26 inc al
> 0x00007FF61C8B5A2F xor bl, al
> 0x00007FF61C8B5A34 movzx rax, al
> 0x00007FF61C8B5A41 mov rdx, [r12+rax*8]
> 0x00007FF61C8B5A49 xor rdx, 0x7F3D2149
> 0x00007FF61C8B5507 inc rsi
> 0x00007FF61C8B7951 add rdx, r13
> 0x00007FF61C8B7954 jmp rdx
> calc_jmp extracted from vm_entry... calc_jmp:
==================================================================================
> 0x00007FF61C8B5A05 mov al, [rsi]
> 0x00007FF61C8B5A0A xor al, bl
> 0x00007FF61C8B5A11 neg al
> 0x00007FF61C8B5A19 rol al, 0x05
> 0x00007FF61C8B5A26 inc al
> 0x00007FF61C8B5A2F xor bl, al
> 0x00007FF61C8B5A34 movzx rax, al
> 0x00007FF61C8B5A41 mov rdx, [r12+rax*8]
> 0x00007FF61C8B5A49 xor rdx, 0x7F3D2149
> 0x00007FF61C8B5507 inc rsi
> 0x00007FF61C8B7951 add rdx, r13
> 0x00007FF61C8B7954 jmp rdx
==================================================================================
> virtual instruction pointer advancement: forward
> located vm handler table... at = 0x00007FF61C8B6473, rva = 0x0000000140006473
> vm handler table entries decrypted with = xor rdx, 0x7F3D2149
> vm handler table entries encrypted with = xor rdx, 0x7F3D2149
==================================================================================
> virtual instruction rva decryption instructions:
        not esi
        neg esi
        ror esi, 0x1A
> virtual instruction rva encryption instructions:
        rol esi, 0x1A
        neg esi
        not esi
==================================================================================
```
