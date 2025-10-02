# VMDevirt - VMProtect Static Devirtualization

VMDevirt is a project which uses LLVM to lift `vmprofiles` to LLVM IR. This lifting aims to be semantically accurate so that the generated native instructions can be executed as normal. This project only supports x86_64 PE binaries.


Currently there is no project to generate IL for vmp3 virtual routines, however when that code has been willed into existence this project will be used to compile the IL back to native. The lifters/vmprofiles for vmp2 and vmp3 are pretty much the same.

# Compiling

<div align="center">
    <img src="https://media1.giphy.com/media/FovFeej5SQQh94uyyK/giphy.gif"/>
    <br>
    <i>LLVM takes forever to build and a few GB's of cache space in tmp...</i>
</div>

### Requirements

* CMake version 3 and above
* Visual Studios 2019 (Fully updated!)
* 16gb+ of RAM
* 10gb of free disk space

### Steps

Clone the entire repo recursively:

`git clone --recursive https://githacks.org/vmp2/vmdevirt.git`

Open a console inside of `vmdevirt` folder and execute the following CMake command:

`cmake -B build` 

# Usage - Generating Native

In order to use this project you must first generate a `vmp2` file using `VMEmu`. This file contains the IL form of every single virtual instruction of every single virtual code block of every single virtualized routine that you generate from. 

In order for VMEmu to work, all virtual instructions in the given virtual routine(s) must be defined. Please refer to the doxygen of `vmprofiler` to learn how to declare a vmprofile.

Once a `vmp2` file is generated you can then provide it to `vmdevirt` along with the virtualized binary. `vmdevirt` will lift all of the IL and compile it back to native, then append it to the virtualized binary and patch all jmp's into the virtualized routines to go into the devirtualized code.
