<div align="center">
    <div>
        <img src="https://imgur.com/koSBEKm.png"/>
    </div>
</div>

# VMProfiler Qt - VMProtect 2 Virtual Instruction Inspector

VMProfiler Qt is a GUI project designed to inspect VMProtect 2 virtual instructions. In order to use this GUI please create a `.vmp2` file using [VMEmu](https://githacks.org/vmp2/vmemu). Once this `.vmp2` has been created you can inspect it by clicking ***File*** ---> ***Open***.

# Building

Simply replace `G:\Qt\5.15.1\msvc2019_64` with the path to your Qt install. Make sure it is of type `2019_64` otherwise it wont work!

```
git clone --recursive https://githacks.org/vmp2/vmprofiler-qt.git
cd vmprofiler-qt
cmake -B build -DCMAKE_PREFIX_PATH=G:\Qt\5.15.1\msvc2019_64
```
