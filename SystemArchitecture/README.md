## Windows System Architecture

Obligatory overview of Windows system architecture.

### System Architecture Overview

It is difficult to find a more succinct overview of Windows system architecture than this image.

![SystemArchitecture](Local/SystemArchitecture.png)

### User Mode Processes

In general, there are four (4) types of user-mode processes on Windows:

- User Processes: standard user applications, 16, 32, or 64 bit
    - ex: _notepad.exe_
- Service Processes: processes that host Windows services; may exist outside of any single user logon session; may be hosted by _svchost.exe_
    - ex: task scheduler
- System Processes: fixed processes that are NOT managed by the SCM; some are considered critical, meaning that terminating said process results in system crash
    - ex: _lsass.exe_
- Environment Subsystem Server Processes: implement the support for the OS subsystem presented to the user; some are considered critical, meaning that terminating said process results in system crash
    - ex: _csrss.exe_
 
### Core Subsystem DLLs

Subsystem DLLs are those DLLs that expose the subsystem API to use mode applications that target that subsystem. They serve as the intermediary between user mode applications and the system support library, _ntdll.dll_.

Windows was originally designed to host multiple _subsystems_, allowing different applications to target different subsystems and thereby access different subsets of the total functionality exported by the OS. However, modern versions of Windows only (really) support a single subsystem: the Windows subsystem. The Windows subsystem is implemented by a number of subsystem DLLs that expose the subsystem API to user mode applications. Four of the most important such DLLs include:

- _kernel32.dll_: exposes most of the Win32 base APIs
- _advapi32.dll_: security calls and functions for manipulating the registry
- _user32.dll_: the user component of Windows GUI rendering and window events
- _gdi32.dll_: graphics device interface, low level primitives for rendering graphics

To really dig into what each of these subsystem DLLs do, one has but to run the four following commands:

```
dumpbin /EXPORTS kernel32.dll
dumpbin /EXPORTS advapi32.dll
dumpbin /EXPORTS user32.dll
dumpbin /EXPORTS gdi32.dll
```
A summary of the results of running this experiment on my system (Windows 10.0.18362) is shown below.

_kernel32.dll_ exports 1603 functions. Examples include:

- `HeapCreate()`
- `OpenProcess()`
- `SetThreadContext()`
- `WriteConsoleA()`

_advapi32.dll_ exports 1857 functions. Examples include:

- `EncryptFileA()`
- `InitializeSecurityDescriptor()`
- `OpenProcessToken()`
- `RegSetValueA()`
- `WmiOpenBlock()`

_user32.dll_ exports 2716 functions (many of which are exported by ordinal only). Examples include:

- `GetForegroundWindow()`
- `MessageBoxA()`
- `SetWindowLongA()`
- `TrackMouseEvent()`

_gdi32.dll_ exports 1016 functions. Examples include:

- `EngFillPath()`
- `GetTextColor()`
- `PolylineTo()`
- `StrokePath()`

The export examples from each subsystem DLL are illustrative of the particular role that each is meant to serve in implementing the Windows subsystem.

**Aside: _kernel32.dll_ vs _kernelbase.dll_**

Starting with Windows 7, Windows introduced a new layer of indirection into core system DLL export resolution and implementation in an attempt to minimize the cost to applications of requesting OS services. The _kernel32.dll_ vs _kernelbase.dll_ distinction is one such example of this change in which the _kernelbase.dll_ binary was introduced in Windows 7 and now contains the implementation of various functions from _kernel32.dll_ (as well as _advapi32.dll_). This implies that the actual function exports from _kernel32.dll_ are in fact forwarded to implementations in _kernelbase.dll_. 

See the experiment "Kernel32Forwarding" for a demonstration of this behavior.

### System Support Library (ntdll.dll)

The system support library, _ntdll.dll_, has the following properties:

- Exports the "native API"
- The lowest level OS interface available to user mode
- Responsible for issuing system calls, transitioning to and from kernel mode
- Supports native applications (those with no subsystem)
- Also used extensively by programs that run in user mode early during system initialization
    - e.g. _csrss.exe_ implements the Windows subsystem, and thus cannot rely on the Windows subsystem, so it must be a native application that utilizes the native API directly
    - e.g. _smss.exe_ managers user sessions and initializes subsystems, and thus cannot rely on the Windows subsystem
 
### Windows Subsystem

The Windows subsystem is the primary subsystem utilized by all modern Windows applications (by this I do not mean UWP apps). 

The environment subsystem process for the Windows subsystem is the Client Server Runtime Subsystem, _csrss.exe_.

### Windows 10 System Processes

Instead of trying to write up my own (inadequate) recreation, I will instead simply link to the [Hunt Evil Poster](Local/SANS_Poster_2018_Hunt_Evil_FINAL.pdf) from SANS that beautifully describes all of the Windows 10 system processes.

**_System_**

**_Secure System_**

**_System Idle_**

**_System Interrupts_**

**_Memory Compression_**

**_Registry_**

**_smss.exe_**

**_wininit.exe_**

**_RuntimeBroker.exe_**

**_taskhostw.exe_**

**_winlogon.exe_**

**_csrss.exe_**

**_services.exe_**

**_svchost.exe_**

**_lsaiso.exe_**

**_lsass.exe_**

**_explorer.exe_**

### References

- _Windows Internals, 7th Edition Part 1_ Pages 45-99
