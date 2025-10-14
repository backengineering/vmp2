# VMProtect 2 Unpacker

To unpack usermode files packed with VMProtect 2 simply use the following command:

```
vmemu --bin [file to unpack] --unpack --out unpacked.bin
```

You can download [precompiled vmemu executables here.](https://github.com/user-attachments/files/22897183/Release.zip) As a note, if you are not running vmemu on a Windows system you must provide it with a `kernel32.dll` for it to map into the emulator.