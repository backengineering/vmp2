# Introduction

This repository is a collection of legacy tools for working with **VMProtect 2**-protected binaries.  
These tools enable unpacking, analysis, and experimental **devirtualization** of software protected with VMProtect 2. At the core of this collection is **`vmprofiler`**, a utility library that provides the foundational logic used by all other tools. It is responsible for identifying key VMProtect 2 components such as **VM handlers**, the **handler table**, **transforms**, and more.

Attempting to identify individual VM handlers is brittle, does not scale, and can be defeated by changes to the virtual machine. Instead, this project advocates an incremental lifting and control‑flow recovery strategy with minimal VM‑specific deobfuscation logic. Tools such as [Saturn](https://arxiv.org/pdf/1909.01752), [Triton](https://github.com/JonathanSalwan/VMProtect-devirtualization), and [Mergen](https://github.com/NaC-L/Mergen) have successfully deobfuscated VMProtect with little or no VM‑specific handler identification. This repository documents the architecture of VMProtect 2 and serves as a cautionary note: avoid building devirtualization tooling that heavily depend on identifying virtual‑machine‑specific handlers.

---

### Tools Overview

- **[vmemu](/vmemu)**  
  A Unicorn Engine–based virtual machine exploration tool.  
  It allows you to **unpack protected programs** and **analyze control flow** within a specified `vmenter`.  
  This tool generates a `.vmp2` file containing the control flow graph of the given VM entry.  
  The resulting `.vmp2` file can then be passed to **[vmdevirt](/vmdevirt)** to recompile the function back into native x86 and reintegrate it into the original binary.

- **[vmdevirt](/vmdevirt)**  
  An **experimental LLVM-based recompiler** for `.vmp2` files.  
  While functional, it is considered **highly experimental and unstable**, and this approach is generally **not recommended**.

- **[vmassembler](/vmassembler)**  
  An **experimental assembler** for encoding user-defined virtual machine operations.  
  This project demonstrates a deep technical understanding of the **VMProtect 2 virtual machine architecture**.

- **[vmhook](/vmhook)**  
  A tool designed to **hook into the VMProtect 2 virtual machine** at runtime, enabling detailed introspection of **individual VM handlers**.

- **[vmprofiler](/vmprofiler)**  
  The **core utility library** that powers all other tools.  
  It includes VM handler pattern matching, handler table extraction, dead code elimination (DCE), and other essential analysis routines.

- **[vmprofiler-cli](/vmprofiler-cli)**  
  A **command-line interface** for `vmprofiler`, allowing users to display and inspect extracted information in a textual format.

- **[vmprofiler-qt](/vmprofiler-qt)**  
  A **Qt-based graphical disassembler** for VMProtect 2 bytecode, providing an interactive and visual exploration of virtualized code.

---

## VMProtect 2 Unpacker

Please follow the [instructions on this page.](UNPACKER.md)

## Build Instructions

### Prerequisites

* CMake ≥ 3.11
* C++ compiler
* Visual Studios 2022 for `vmhook`
* At least 50GB free space for LLVM and other build artifacts

**Configuration for vmassembler, vmemu, vmprofiler, and vmprofiler-cli:**

```bash
cmake -B build
cmake --build build
```

**With vmdevirt (will download and build LLVM!):**

```bash
cmake -B build -DBUILD_VMDEVIRT=ON -DLLVM_BUILD_TYPE=Release
cmake --build build
```

**With vmprofiler-qt:**

```bash
cmake -B build -DBUILD_VMPROFILER_QT=ON
cmake --build build
```

**Both optional modules:**

```bash
cmake -B build -DBUILD_VMDEVIRT=ON -DBUILD_VMPROFILER_QT=ON -DLLVM_BUILD_TYPE=Release
cmake --build build
```

# Legal Disclaimer

> These tools are provided for educational and research purposes only. They are not to be used for any illegal activities, unauthorized access, or violations of the DMCA or other laws. The tools themselves do not violate any DMCA regulations. All work is my original creation, and any actions taken by users of these tools are solely the responsibility of those users and do not reflect the actions or intentions of the author.