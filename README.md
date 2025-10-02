## Build Instructions

### Prerequisites

* CMake ≥ 3.11
* C++ compiler
* Visual Studios for `vmhook`

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