## Experiment: WoW64 System Call Interception

In this experiement, we will look at the Windows on Windows64 (WoW64) mechanisms that transparently allow a 32-bit application to invoke system calls on 64-bit systems.

### System Information

This experiment was performed on a system with the following specifications:

```
OS Name:                   Microsoft Windows 10 Pro for Workstations
OS Version:                10.0.18362 N/A Build 18362
System Type:               x64-based PC
```

### Building the Simple Test Program `DebugMe`

Build two (2) versions of the `DebugMe` program - a 32-bit program and a 64-bit program.

The 32-bit version may be built by running the following command from the x86 Native Tools command prompt:

```
cl /EHsc /nologo /Fe:DebugMe86 DebugMe.cpp /link /machine:x86
```

And similarly, the 64-bit version may be built by running the following command from the x64 Native Tools command prompt:

```
cl /EHsc /nologo /Fe:DebugMe64 DebugMe.cpp /link /machine:x64
```

Strictly speaking, the `/machine` flag is not necessary in either case here because we are invoking the compiler and the linker in the same step and we don't have a cross-compiler setup available.

We can confirm that the two versions of the executable have been built correctly by examining the PE headers with the help of the `dumpbin` utility. 

```
> dumpbin /headers DebugMe86.exe
Dump of file DebugMe86.exe

PE signature found

File Type: EXECUTABLE IMAGE

FILE HEADER VALUES
             14C machine (x86)
               4 number of sections
        5E1B9605 time date stamp Sun Jan 12 16:56:21 2020
               0 file pointer to symbol table
               0 number of symbols
              E0 size of optional header
             102 characteristics
                   Executable
                   32 bit word machine

OPTIONAL HEADER VALUES
             10B magic # (PE32)
             ...
```

The `machine` value (`14C`) in the File Header and the `magic` value (`10B`) in the Optional Header confirm that this is indeed a 32-bit executable. Similarly, for the 64-bit version:

```
> dumpbin /headers DebugMe64.exe
Dump of file DebugMe64.exe

PE signature found

File Type: EXECUTABLE IMAGE

FILE HEADER VALUES
            8664 machine (x64)
               6 number of sections
        5E1B95D2 time date stamp Sun Jan 12 16:55:30 2020
               0 file pointer to symbol table
               0 number of symbols
              F0 size of optional header
              22 characteristics
                   Executable
                   Application can handle large (>2GB) addresses

OPTIONAL HEADER VALUES
             20B magic # (PE32+)
             ...
```

As before, the `machine` value (`8664`) in the File Header and the `magic` value (`20B`) in the Optional Header confirm that this is indeed a 64-bit executable.

### Debugging the Test Application

First, lets just look at some of the differences between the modules loaded by the two applications. Once the initial image loader breakpoint is hit, we can examine the loaded modules.

Loaded modules for _DebugMe86.exe_:

```
0:000> lm
start    end        module name
000e0000 0011a000   DebugMe86 C (no symbols)           
74bd0000 74c6f000   apphelp    (pdb symbols)          c:\localsymbols\apphelp.pdb\0D37A746D2E21547F2B11D6457231F8D1\apphelp.pdb
75f30000 76010000   KERNEL32   (pdb symbols)          c:\localsymbols\wkernel32.pdb\7D80824F9CCE7C819044B16FD421C63D1\wkernel32.pdb
769f0000 76bec000   KERNELBASE   (pdb symbols)          c:\localsymbols\wkernelbase.pdb\8BC9719D7B1E26272FB1CB98D403792C1\wkernelbase.pdb
774b0000 7764a000   ntdll      (pdb symbols)          c:\localsymbols\wntdll.pdb\D85FCE08D56038E2C69B69F29E11B5EE1\wntdll.pdb
```

Loaded modules for _DebugMe64.exe_:

```
0:000> lm
start             end                 module name
00007ff6`e7c10000 00007ff6`e7c5b000   DebugMe64 C (no symbols)           
00007ff8`53310000 00007ff8`5339f000   apphelp    (pdb symbols)          c:\localsymbols\apphelp.pdb\B37F976CBE765DAE97B61DFD29AED8C51\apphelp.pdb
00007ff8`55330000 00007ff8`555d3000   KERNELBASE   (pdb symbols)          c:\localsymbols\kernelbase.pdb\1773CB342642E1A70A5E70D8D2A32BD31\kernelbase.pdb
00007ff8`575f0000 00007ff8`576a2000   KERNEL32   (pdb symbols)          c:\localsymbols\kernel32.pdb\5A77DE8CE8D58731F0EA38F1C92F48D81\kernel32.pdb
00007ff8`58300000 00007ff8`584f0000   ntdll      (pdb symbols)          c:\localsymbols\ntdll.pdb\0C2E19EA1901E9B82E4567D2D21E56D21\ntdll.pdb
```

So the modules loaded by each application appear identical, but are they?

```
0:000> lm Dvm kernel32
Browse full module list
start    end        module name
75f30000 76010000   KERNEL32   (pdb symbols)          c:\localsymbols\wkernel32.pdb\7D80824F9CCE7C819044B16FD421C63D1\wkernel32.pdb
    Loaded symbol image file: C:\WINDOWS\System32\KERNEL32.DLL
    Image path: C:\WINDOWS\SysWOW64\KERNEL32.DLL
    Image name: KERNEL32.DLL
```

```
0:000> lm Dvm kernel32
Browse full module list
start             end                 module name
00007ff8`575f0000 00007ff8`576a2000   KERNEL32   (pdb symbols)          c:\localsymbols\kernel32.pdb\5A77DE8CE8D58731F0EA38F1C92F48D81\kernel32.pdb
    Loaded symbol image file: C:\WINDOWS\System32\KERNEL32.DLL
    Image path: C:\WINDOWS\System32\KERNEL32.DLL
    Image name: KERNEL32.DLL
```

Looking a little closer reveals that the 32-bit version of the application loads _kernel32.dll_ from the `Windows\SysWoW64` directory, while the 64-bit version of the application loads it from the `Windows\System32` directory, as expected.

However, performing the same test for _ntdll.dll_ reveals that both versions of the application load the binary from the `Windows\System32` directory. This finding seems to contradict the architectural description of WoW64 described in Windows Internals.

If we trace the system call performed by the test applciation (`CreateEventW()`) we begin to see some evidence of the additional machinery required to support the WoW64 illusion. For instance, at a relatively early point in the call stack for the system service call we hit the following `call` instruction:

```
0:000> pct
eax=00000048 ebx=00000000 ecx=00000000 edx=77538d30 esi=00000001 edi=00f83850
eip=775221fa esp=00bbf700 ebp=00bbf754 iopl=0         nv up ei pl zr na pe nc
cs=0023  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00000246
ntdll!NtCreateEvent+0xa:
775221fa ffd2            call    edx {ntdll!Wow64SystemServiceCall (77538d30)}
```

This suggests that there is some logic present to differentiate between system service calls originating from 32-bit and 64-bit applications, and that those originating from 32-bit applications hit this call which (soon thereafter) invokes the following `jmp` instruction:

```
0:000> t
eax=00000048 ebx=00000000 ecx=00000000 edx=77538d30 esi=00000001 edi=00f83850
eip=77538d30 esp=00bbf6fc ebp=00bbf754 iopl=0         nv up ei pl zr na pe nc
cs=0023  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00000246
ntdll!Wow64SystemServiceCall:
77538d30 ff2528125d77    jmp     dword ptr [ntdll!Wow64Transition (775d1228)] ds:002b:775d1228=774a6000
```

Further analysis is warranted to determine precisely the operations that this function performs to actually perform the transition to the native system call.

