## Experiment: Invalid Addresses

In this experiment we explore the concrete reason that address 0 (the lowest address available in the user mode address space) is invalid or unable to be referenced.

### System Information

This experiment was performed on a system with the following specifications:

```
OS Name:                   Microsoft Windows 10 Pro for Workstations
OS Version:                10.0.18362 N/A Build 18362
System Type:               x64-based PC
```

### Building the Test Application

Build the test application by invoking the following command from an x64 Native Tools command prompt:

```
cl /EHa /nologo InvalidAddress.cpp /link /debug
```

The `/EHa` flag enables SEH exception handling (in conjunction with C++ exceptions) and the `/debug` linker flag instructs the linker to produce a program database (.pdb) file that will improve the debugging experience.

### Debugging the Test Application

Naturally, running the test application under a debugger triggers a first-chance exception breakpoint when the null pointer (0) is dereferenced. The debugger reports this violation as follows:

```
(3104.280c): Access violation - code c0000005 (first chance)
First chance exceptions are reported before any exception handling.
This exception may be expected and handled.
InvalidAddress!main+0x13:
00007ff7`f3306ba3 c70001000000    mov     dword ptr [rax],1 ds:00000000`00000000=????????
```

Exactly as one might expect, an access violation is raised by the attempt to write to address 0.

```
0:000> bp kernelbase!LoadLibraryExW
0:000> bl
     0 e Disable Clear  00007ff8`55355560     0001 (0001)  0:**** KERNELBASE!LoadLibraryExW
```

Now the question is, will the SEH mechanics allow the program to continue execution after this violation has occurred? Allowing execution to continue after the exception occurs reveals that this is indeed the case.

I noticed during this process that execution ends up in the image loader for some reason. Why? In an attempt to answer this question, I re-ran the program under the debugger and set a breakpoint at one of the Win32 entries to the image loader.

```
0:000> bp kernelbase!LoadLibraryExW
0:000> bl
     0 e Disable Clear  00007ff8`55355560     0001 (0001)  0:**** KERNELBASE!LoadLibraryExW
```

Now, after the exception is raised and we proceed with execution, the breakpoint is hit.

```
0:000> g
Breakpoint 0 hit
KERNELBASE!LoadLibraryExW:
00007ff8`55355560 4055            push    rbp
```

What library is being loaded? The first argument to `LoadLibraryExW()` is a unicode string representing the name of the module being loaded. This argument, per the Windows x64 calling convention, is passed to the function via the `rcx` register. Thus we have all the information we need to determine the library that is loaded in response to the exception.

```
0:000> r
rax=000000000000001d rbx=0000000000000000 rcx=00007ff7f3371e70
rdx=0000000000000000 rsi=0000000000000012 rdi=ffffffffffffffff
rip=00007ff855355560 rsp=000000dd18fefa88 rbp=00007ff7f33722bc
 r8=0000000000000800  r9=00007ff7f33722c0 r10=0000cdbb5b782e1d
r11=000000dd18fefa10 r12=00007ff7f33722c0 r13=00007ff7f33722c0
r14=00007ff7f3371e70 r15=000000000000001c
iopl=0         nv up ei pl zr na po nc
cs=0033  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00000246
KERNELBASE!LoadLibraryExW:
00007ff8`55355560 4055            push    rbp
0:000> du 00007ff7f3371e70
00007ff7`f3371e70  "api-ms-win-appmodel-runtime-l1-1"
00007ff7`f3371eb0  "-2"
```

So the _api-ms-win-appmodel-runtime-l1-1-2.dll_ module is loaded in response to the exception.

### Further Research

I have yet to find a good resource describing the purpose and function of this module. It is not located in the redistributable runtime library directory as I expected it to be. 