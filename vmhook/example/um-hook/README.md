<div align="center">
    <div>
        <img src="https://back.engineering/vmprotect-2/7.png"/>
    </div>
</div>

# um-hook - Usermode Virtual Instruction Hook Demo

um-hook is a demo project/repo which contains an example of how to hook a virtual instruction and alter its results. The binaries in this repo are not packed for simplicity sake, however in later demo's I will be applying packing to the executable. 

#### Contents

* dependencies/ - this project is dependent on `vmhook`. 
* refbuilds/ - binaries protected with `ultra virtualization` and no packing. These bins are for you to mess with!
* src/ - source code for the usermode tracer. 
    * vmptest/ - source code for the the test bins
    * um-hook/ - source code for usermode hook, includes a hook on `LCONSTBZX`.

### Usage

First download the repo with `git clone --recursive https://githacks.org/vmp2/um-hook.git`, then compile the `um-hook` by opening `um-hook.sln` inside of `src/`. There should be an executable called `um-hook.exe` in `x64/Release`. This hook demo program is compiled for the first `vmptest` binary in the `refbuilds` directly.

To create a trace file simply run the following:

```
um-hook.exe --bin vmptest.vmp.exe --table 0x6473 --base 0x140000000
```