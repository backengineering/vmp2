<div align="center">
    <div>
        <img src="https://githacks.org/vmp2/vmassembler/-/raw/2044541ce3c4608186dc687b048c01191f0f4ea7/doxygen/icon.png"/>
    </div>
</div>

# VMAssembler - VMProtect 2 Virtual Instruction Assembler

VMAssembler is a small C++ project which uses [flex](https://en.wikipedia.org/wiki/Flex_(lexical_analyser_generator)) and [bison](https://www.gnu.org/software/bison/manual/) to parse `.vasm` files and assemble virtual instructions. The project inherits [vmprofiler](https://githacks.org/vmp2/vmprofiler) which is used to identify vm handler's, provide them with a name, immidate value size, and other meta data to aid in assembling virtual instructions.

# [Doxygen Documentation](https://docs.back.engineering/vmassembler/)

### Contents 

The repo contains the following notable folders and files:

* `dependencies/` - vmprofiler is the only dependency for this project...
* `src/` - source code for the vmassembler...
    * `compiler.cpp` - responsible for encoding and encrypting virtual instructions...
    * `parser.cpp` - a singleton class which is used in `parser.y`...
    * `parser.y` - bison rules for parsing tokens. This contains only a handful of rules...
    * `lexer.l` - lex rules for the vmassembler...

### Usage Requirements

In order to use the virtual instruction assembler you must first have a few values at hand. You must have an unpacked vmprotect 2 binary. This file cannot be a dump, it must be unpacked. Second, you must know the RVA to the vm entry address of the first push instruction. This first push instruction must not be `push contant_value`, it must be of type `push reg` as the constant values are pushed onto the stack by the generated c++ header file. The last thing you need is a virtual instruction assembly file. You can make one of these easily. Simply open a text editor and create your first label:


***Note: you can use ANY virtual instruction which is defined by vmprofiler... you can see them all [here](https://githacks.org/vmp2/vmprofiler/-/tree/a419fa4633f98e2f819b1119dc0884154e207482/src/vmprofiles)***

```
get_hello:
	SREGQ 0x90
	SREGQ 0x88
	SREGQ 0x80
	SREGQ 0x78
	SREGQ 0x70
	SREGQ 0x68
	SREGQ 0x60
	SREGQ 0x58
	SREGQ 0x50
	SREGQ 0x48
	SREGQ 0x40
	SREGQ 0x38
	SREGQ 0x30
	SREGQ 0x28
	SREGQ 0x20
	SREGQ 0x18
	SREGQ 0x10
	SREGQ 0x8
	SREGQ 0x0
	
	LCONSTQ 0x6F6C6C6568
	SREGQ 0x78
	SREGQ 0x0
	SREGQ 0x0

	LREGQ 0x0
	LREGQ 0x8
	LREGQ 0x10
	LREGQ 0x18
	LREGQ 0x20
	LREGQ 0x28
	LREGQ 0x30
	LREGQ 0x38
	LREGQ 0x40
	LREGQ 0x48
	LREGQ 0x50
	LREGQ 0x58
	LREGQ 0x60
	LREGQ 0x68
	LREGQ 0x70
	LREGQ 0x78
	LREGQ 0x80
	LREGQ 0x88
	LREGQ 0x90
	VMEXIT
	
get_world:
	SREGQ 0x90
	SREGQ 0x88
	SREGQ 0x80
	SREGQ 0x78
	SREGQ 0x70
	SREGQ 0x68
	SREGQ 0x60
	SREGQ 0x58
	SREGQ 0x50
	SREGQ 0x48
	SREGQ 0x40
	SREGQ 0x38
	SREGQ 0x30
	SREGQ 0x28
	SREGQ 0x20
	SREGQ 0x18
	SREGQ 0x10
	SREGQ 0x8
	SREGQ 0x0
	
	LCONSTQ 0x646C726F77
	SREGQ 0x78
	SREGQ 0x0
	SREGQ 0x0

	LREGQ 0x0
	LREGQ 0x8
	LREGQ 0x10
	LREGQ 0x18
	LREGQ 0x20
	LREGQ 0x28
	LREGQ 0x30
	LREGQ 0x38
	LREGQ 0x40
	LREGQ 0x48
	LREGQ 0x50
	LREGQ 0x58
	LREGQ 0x60
	LREGQ 0x68
	LREGQ 0x70
	LREGQ 0x78
	LREGQ 0x80
	LREGQ 0x88
	LREGQ 0x90
	VMEXIT
```

# Usage Example

Once you have defined a vasm file, you can now generate the c++ header file which will handle everything for you. Simply execute the following command:

```
vmassembler.exe --input [filename] --vmpbin [vmprotect'ed binary] --vmentry [make sure this rva is correct!] --out test.hpp
```

If the file is generated without any errors you can now include this file inside of your project. This header file uses no STL, nor any CRT functions, however it does create a RWX section so if you want to use this in a driver it cannot run on HVCI systems... 


To call your vasm routine all you must do is pass the label name as a template param...

```cpp
#include <iostream>
#include "test.hpp"

int main()
{
    // note, the header file generates an enum call "calls", inside of this enum will be an entry with the same name as your label!
    // note, the second template param is the return type...
    const auto hello = vm::call< vm::calls::get_hello, vm::u64 >();
    const auto world = vm::call< vm::calls::get_world, vm::u64 >();
    std::printf( "> %s %s\n", ( char * )&hello, (char*)&world );
}
```