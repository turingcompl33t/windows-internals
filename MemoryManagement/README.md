## Windows Memory Management

Memory management on Windows systems is controlled by the Memory Manager - the largest executive component.

### Memory Management Terminology

Definition of some common memory management terms, some of which are Windows-specific.

- _Reserved Memory_: reserved memory is memory that may not be allocated to other uses, but is not yet backed by physical resources (e.g. physical memory or page file)
- _Committed Memory_: committed memory is memory that is backed by physical resources (e.g. physical memory or page file)
- _System Commit Limit_: the total size of all paging files plus the total amount of RAM usable by the operating system
- _Canonical Address_: a virtual address that conforms to the limitation imposed by hardware manufacturers and memory addressing limitations; currently, modern hardware only supports 48 address lines, meaning that only the low 48 bits of a virtual address are utilized for addressing while the remaining 16 high order bits must sign extend (be set to the same value as bit 47 of the 64-bit address)

### Memory Manager Services and APIs

There are various ways to access the services exposed by the Memory Manager from user mode applications.

- Local / Global APIs: Functions like `LocalAlloc()` and `GlobalAlloc()` are leftovers from previous versions of Windows and have been re-implemented to utilize the heap API under the hood.
- C/C++ Runtime: Functions like `malloc()`, `free()`, `operator new`, and `operator delete` (typically) call the Windows heap API under the hood to perform dynamic memory allocation (compiler-dependent).
- Heap API: Functions like `HeapAlloc()` and `HeapFree()` are intended for managing "smaller" memory allocations (e.g. less than a page); the heap API utilizes the virtual API under the hood.
- Virtual API: Functions like `VirtualAlloc()`, `VirtualFree()`, and `VirtualProtect()` comprise the lowest level of user mode memory management routines; these APIs always operate at page granularity.
- Memory Mapping API: Functions like `CreateFileMapping()` and `MapViewOfFile()` allow user mode applications to map files directly into virtual address space; these functions do not make use of the virtual API.

### Memory Management General Parameters

**Virtual Address Space Sizes**

The size of the virtual address space for a process on a Windows system varies depending on a number of factors. Excluding more complex factors, the basics are as follows:

32-bit Process on x86 Platform

- Default: 2GB
- Large Address Space Aware: 3GB

32-bit Process on x64 Platform

- Default (?): 4GB

64-bit Process on x64 Platform

- Default (Windows 8, Server 2012): 8TB
- Default (Windows 8.1, Server 2012 R2): 128TB

**Page Sizes**

| Architecture | Small Page Size | Large Page Size | Small Pages Per Large Page |
|--------------|-----------------|-----------------|----------------------------|
| x86          | 4KB             | 2MB             | 512                        |
| x64          | 4KB             | 2MB             | 512                        |

**Allocation Granularity**

The allocation granularity for the system may be obtained by invoking the `GetSystemInfo()` API. The allocation granularity is 64KB, meaning that Windows aligns each region of reserved virtual address space on a 64KB boundary. 

**Memory Protection Options**

The following values describe the memory protection attributes available via the Windows API.

- `PAGE_NOACCESS`
- `PAGE_READONLY`
- `PAGE_READWRITE`
- `PAGE_EXECUTE`
- `PAGE_EXECUTE_READ`
- `PAGE_EXECUTE_READWRITE`
- `PAGE_WRITECOPY`
- `PAGE_EXECUTE_WRITECOPY`
- `PAGE_GUARD`
- `PAGE_NOCACHE`
- `PAGE_WRITECOMBINE`
- `PAGE_TARGETS_INVALID` and `PAGE_TARGETS_NO_UPDATE`

### User Mode Virtual Address Space

The user mode virtual address space for processes is divided into a few well-defined regions, including:

- Process Executable Image (memory-mapped file)
- Loaded Modules (memory-mapped files)
- Default Process Heap
- Other Heaps
- Default Thread Stack
- Other Thread Stacks

With ASLR enabled, the load address of the process executable image and dynamic-base DLLs, and the location of the default process heap are all randomized (to a certain degree). For images, the randomly selected load address is always within a 16MB delta of the preferred load address present in the PE header, and is always 64KB aligned.

The story is slightly different for DLLs loaded by the process. Because DLLs are meant to be shared among many user mode processes (memory mapped into many virtual address spaces) the randomized load address for a DLL is computed at boot time (through a value called the _image bias_) and remains consistent across the boot.

ASLR also randomizes the base address of thread stacks within the process virtual address space. The randomized base location is again based on a CPU timestamp, but this time from a value that is generated at process initialization time rather than at boot.

ASLR performs a similar procedure for randomizing the base address of all process heaps.

The Sysinternals _vmmap.exe_ utility is a great tool for exploring the virtual address space of user mode processes.

### Memory Management API

The Windows user mode memory management API consists of the functions provided in the `memoryapi.h` header.

### References

- _Windows Internals, 7th Edition Part 1_ Pages 301-482
