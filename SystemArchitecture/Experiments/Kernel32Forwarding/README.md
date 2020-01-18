## Experiment: _kernel32_ Export Forwarding and Trampolines

In this experiment, we will explore the export forwaring performed by the core system DLL _kernel32.dll_. Specifically, we will look at an export defined by _kernel32.dll_ that is forwarded to the _kernelbase.dll_ library introduced in Windows 7.

### System Information

This experiment was performed on a system with the following specifications:

```
OS Name:                   Microsoft Windows 10 Pro for Workstations
OS Version:                10.0.18362 N/A Build 18362
System Type:               x64-based PC
```

### A Failed First Attempt

The first step will be to locate an export that is shared by _kernel32.dll_ and _kernelbase.dll_. One way to accomplish this is via the `dumpbin` utility. Both DLLs are located in the `C:\Windows\System32\` directory.

```
> dumpbin kernel32.dll /exports
...
> dumpbin KernelBase.dll /exports
```

Running these commands produces quite a bit of output:
- _kernel32.dll_ exports 1629 functions
- _kernelbase.dll_ exports 1887 functions

However, finding a function that is exported by both modules is not particularly difficult. For instance, lets search for the ubiquitous `CreateFile()` function in both libraries.

```
> dumpbin kernel32.dll /exports | findstr CreateFile
         26   19          AppPolicyGetCreateFileAccess (forwarded to kernelbase.AppPolicyGetCreateFileAccess)
        195   C2 00022070 CreateFile2
        196   C3 00022080 CreateFileA
        197   C4 0001AB30 CreateFileMappingA
        198   C5          CreateFileMappingFromApp (forwarded to api-ms-win-core-memory-l1-1-1.CreateFileMappingFromApp)
        199   C6 0005AB50 CreateFileMappingNumaA
        200   C7 000357E0 CreateFileMappingNumaW
        201   C8 0001C250 CreateFileMappingW
        202   C9 0005A100 CreateFileTransactedA
        203   CA 0005A1C0 CreateFileTransactedW
        204   CB 00022090 CreateFileW
        953  3B8 00037210 LZCreateFileW
> dumpbin KernelBase.dll /exports | findstr CreateFile
         49   2F 000DD860 AppPolicyGetCreateFileAccess
        186   B8 00074540 CreateFile2
        187   B9 00024170 CreateFileA
        188   BA 000FAE60 CreateFileMapping2
        189   BB 000809C0 CreateFileMappingFromApp
        190   BC 0002CAE0 CreateFileMappingNumaW
        191   BD 0002CDA0 CreateFileMappingW
        192   BE 00024270 CreateFileW
```

Here we see that `CreateFileW()` is defined as an export for both libraries. Perhaps the implementation of `CreateFileW()` in _kernel32.dll_ is simply a trampoline to the implementation in _kernelbase.dll_? Debugging an application with `windbg` allows us to find out.

Launch `windbg` and launch an executable (I chose _notepad.exe_). As a first step, verify that both modules are loaded by this application.

```
0:000> lm
start             end                 module name
00007ff6`21c60000 00007ff6`21c92000   notepad    (pdb symbols)          
00007ff8`55330000 00007ff8`555d3000   KERNELBASE   (pdb symbols)          c:\localsymbols\kernelbase.pdb\1773CB342642E1A70A5E70D8D2A32BD31\kernelbase.pdb
...
00007ff8`575f0000 00007ff8`576a2000   KERNEL32   (pdb symbols)          c:\localsymbols\kernel32.pdb\5A77DE8CE8D58731F0EA38F1C92F48D81\kernel32.pdb
```

So both modules are indeed loaded. Next we can simply unassemble the implementations of `CreateFileW()` in both modules and compare the output.


```
uf kernel32!CreateFileW
...
uf kernelbase!CreateFileW
```

It is immediately obvious from the unassembly output that the implementations are dictinct and moreover that the implementation in _kernel32.dll_ is definitely not merely a trampoline to the implementation in _kernelbase.dll_. Back to the drawing board.

### Paying Attention

If we pay attention, the output of the `dumpbin` command earlier for _kernel32.dll_ reveals that certain routines are marked as "forwarded to <library name>":

```
> dumpbin kernel32.dll /exports | findstr CreateFile
         26   19          AppPolicyGetCreateFileAccess (forwarded to kernelbase.AppPolicyGetCreateFileAccess)
        ...
        198   C5          CreateFileMappingFromApp (forwarded to api-ms-win-core-memory-l1-1-1.CreateFileMappingFromApp)
        ...
```

Perhaps these are the functions that we should be looking for, instead of simply assuming that duplicated exports are forwarded.

```
> dumpbin kernel32.dll /exports | findstr /C:"(forwarded to kernelbase"
         25   18          AppPolicyGetClrCompat (forwarded to kernelbase.AppPolicyGetClrCompat)
         26   19          AppPolicyGetCreateFileAccess (forwarded to kernelbase.AppPolicyGetCreateFileAccess)
        ...
       1128  467          RaiseFailFastException (forwarded to kernelbase.RaiseFailFastException)
       1133  46C          ReadConsoleInputExA (forwarded to kernelbase.ReadConsoleInputExA)
       1134  46D          ReadConsoleInputExW (forwarded to kernelbase.ReadConsoleInputExW)
        ...
```

This output reveals a number of functions that are declared as forwarded to _kernelbase.dll_. Searching the exports of _kernelbase.dll_ confirms that it does indeed export these functions.

```
> dumpbin KernelBase.dll /exports | findstr RaiseFailFastException
       1313  520 000FF060 RaiseFailFastException
```

Attempting to unassemble this function in the debugger results in failure for _kernel32.dll_:

```
0:000> u kernel32!RaiseFailFastException
Couldn't resolve error at 'kernel32!RaiseFailFastException'
```

However, as one might expect, the same command succeeds for _kernelbase.dll_:

```
0:000> u kernelbase!RaiseFailFastException
KERNELBASE!RaiseFailFastException:
00007ff8`5542f060 48895c2418      mov     qword ptr [rsp+18h],rbx
00007ff8`5542f065 4889742420      mov     qword ptr [rsp+20h],rsi
00007ff8`5542f06a 57              push    rdi
00007ff8`5542f06b 4881ecc0050000  sub     rsp,5C0h
00007ff8`5542f072 488b05b79d1600  mov     rax,qword ptr [KERNELBASE!_security_cookie (00007ff8`55598e30)]
00007ff8`5542f079 4833c4          xor     rax,rsp
00007ff8`5542f07c 48898424b0050000 mov     qword ptr [rsp+5B0h],rax
00007ff8`5542f084 418bf0          mov     esi,r8d
```

So we see that the forwarding mechanism does not introduce any kind of function trampoline that must be passed through at runtime, and instead completely resolves the forwarded function reference at load time.

### Solving the Mystery (With Some Simple Dynamic Analysis)

I was unsatisfied with the above analysis because it did not answer the question: why are functions like `CreateFileW()` exported by both _kernel32.dll_ and _kernelbase.dll_? In an effort to answer this question, I wrote my own small application strictly for the purpose of running it under a debugger and examining its behavior. The application simply sets a debug breakpoint immediately before a call to `CreateFileW()` allowing for easy debugger interaction.

When the program is loaded by the debugger, the first thing we verify is that both modules of interest are indeed loaded.

```
0:000> lm
start             end                 module name
00007ff6`94190000 00007ff6`941db000   Kernel32Forwarding C (no symbols)           
00007ff8`55330000 00007ff8`555d3000   KERNELBASE   (pdb symbols)          c:\localsymbols\kernelbase.pdb\1773CB342642E1A70A5E70D8D2A32BD31\kernelbase.pdb
00007ff8`575f0000 00007ff8`576a2000   KERNEL32   (pdb symbols)          c:\localsymbols\kernel32.pdb\5A77DE8CE8D58731F0EA38F1C92F48D81\kernel32.pdb
00007ff8`58300000 00007ff8`584f0000   ntdll      (pdb symbols)          c:\localsymbols\ntdll.pdb\0C2E19EA1901E9B82E4567D2D21E56D21\ntdll.pdb
```

Module loads and symbols are good to go, so we can now set a breakpoint on the two exports of interest.

```
0:000> bp kernel32!CreateFileW
0:000> bp kernelbase!CreateFileW
0:000> bl
     0 e Disable Clear  00007ff8`57612090     0001 (0001)  0:**** KERNEL32!CreateFileW
     1 e Disable Clear  00007ff8`55354270     0001 (0001)  0:**** KERNELBASE!CreateFileW
```

Now when we continue execution, we can observe which of these functions is invoked by the top level Win32 call to `CreateFileW()`.

```
0:000> g
Breakpoint 0 hit
KERNEL32!CreateFileW:
00007ff8`57612090 ff2582590500    jmp     qword ptr [KERNEL32!_imp_CreateFileW (00007ff8`57667a18)] ds:00007ff8`57667a18={KERNELBASE!CreateFileW (00007ff8`55354270)}
```

So the implementation in _kernel32.dll_ is the first function hit. However, the disassembly shows that the first instruction executed is an indirect jump to the address located at `KERNEL32!_imp_CreateFileW (00007ff857667a18)`. The debugger has already done the work for us, but we can we can use the dump command to manually examine the address that will be jumped to:

```
0:000> dq 00007ff857667a18 L1
00007ff8`57667a18  00007ff8`55354270
```

If we attempt to disassmble the code at this address, we see the following:

```
0:000> uf 00007ff8`55354270
KERNELBASE!CreateFileW:
00007ff8`55354270 4883ec58        sub     rsp,58h
00007ff8`55354274 448b942488000000 mov     r10d,dword ptr [rsp+88h]
00007ff8`5535427c 418bc2          mov     eax,r10d
00007ff8`5535427f 25b77f0000      and     eax,7FB7h
00007ff8`55354284 c744243020000000 mov     dword ptr [rsp+30h],20h
```

Thus, the first instruction of the _kernel32.dll_ `CreateFileW()` export is simply an indirect jump to the implementation exported by _kernelbase.dll_ - the _kernel32.dll_ implementation merely serves as a trampoline into the _kernelbase.dll_ implementation. Indeed, we can confirm this conclusion by continuing to step through the application in the debugger: 

```
0:000> t
Breakpoint 1 hit
KERNELBASE!CreateFileW:
00007ff8`55354270 4883ec58        sub     rsp,58h
```

The _kernelbase.dll_ implementation is immediately hit.

### Lesssons Learned

I made the mistake in the first section of this experiment by not paying close enough attention to the disassembly output from the debugger. I saw that the _kernel32.dll_ implementation of `CreateFileW()` was an indirect jump, but instead of following the indirection, I instead disassembled the code at the location that holds the jump target and concluded that this was merely a completely distinct implementation of `CreateFileW()` instead of the garbage that it actually is:

```
0:000> u KERNEL32!_imp_CreateFileW
KERNEL32!_imp_CreateFileW:
00007ff8`57667a18 7042            jo      KERNEL32!_imp_UnlockFile+0x4 (00007ff8`57667a5c)
00007ff8`57667a1a 3555f87f00      xor     eax,7FF855h
00007ff8`57667a1f 007041          add     byte ptr [rax+41h],dh
00007ff8`57667a22 3555f87f00      xor     eax,7FF855h
00007ff8`57667a27 0030            add     byte ptr [rax],dh
00007ff8`57667a29 f63a            idiv    byte ptr [rdx]
00007ff8`57667a2b 55              push    rbp
00007ff8`57667a2c f8              clc
```

This is not code at all - we can see that the opcodes for the first two instructions are actually the address of `kernelbase!CreateFileW`. 

Thus, the lesson learned here for me (aside from the actual mechanics of forwarding and export trampolines) is that if we ask the debugger to disassemble some code for us, it will oblige, regardless of whether or not such an operation is appropriate!