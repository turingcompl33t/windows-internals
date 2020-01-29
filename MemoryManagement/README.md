## Windows Memory Management

Memory management on Windows systems is controlled by the Memory Manager - the largest executive component.

### Memory Management Terminology

Definition of some common memory management terms, some of which are Windows-specific.

- _Reserved Memory_: reserved memory is memory that may not be allocated to other uses, but is not yet backed by physical resources (e.g. physical memory or page file)
- _Committed Memory_: committed memory is memory that is backed by physical resources (e.g. physical memory or page file)
- _System Commit Limit_: the total size of all paging files plus the total amount of RAM usable by the operating system
- _Canonical Address_: a virtual address that conforms to the limitation imposed by hardware manufacturers and memory addressing limitations; currently, modern hardware only supports 48 address lines, meaning that only the low 48 bits of a virtual address are utilized for addressing while the remaining 16 high order bits must sign extend (be set to the same value as bit 47 of the 64-bit address)

### Memory Management Architecture

The Windows virtual memory model is implemented by the Memory Manager - a subsystem of the Windows Executive. The Memory Manager directly exposes a set of Executive system services for managing virtual memory; a large subset of these routines are documented and exposed to user-mode and kernel-mode consumers. 

The Memory Manager also implements six system-level routines that constantly run in the background within the context of the `System` process:
- Balance Set Manager: tunes memory-management policy-level configuration at runtime in response to current and past operating conditions
- Process / Stack Swapper: performs the memory management functions required to implement a context switch between processes or threads
- Modified Page Writer: writes dirty pages (identified by their presence on the global, system-maintained "modified page list") back to their appropriate location in one of the paging files
- Mapped Page Writer: writes dirty mapped-file pages back to their appropriate location on disk
- Segment Dereference Thread: manages the expansion and contraction of system page files 
- Zero Page Thread: zeroes out pages that are currently located on the free page list so that they may be utilized to service future demand-zero page faults

The lowest-level virtual memory management routines are ultimately implemented in the body of the Memory Manager within the Windows Executive which executes in kernel-mode context. However, the Memory Manager supports a number of higher-level memory management abstractions on top of the Executive service routines that it exports. It is these abstractions that are utilized by user-mode applications to manage virtual memory within their process address spaces. These higher-level memory management APIs include the following:
- File Mapping API: the file mapping API grants user-mode processes the ability to map the content of filesystem files directly into their virtual address space; the file mapping API also enables inter-process shared memory via memoroy mappings backed by a system page file
- Virtual API: the virtual API is the lowest level API for virtual memory allocation and deallocation; the virtual API only supports memory operations in terms of relatively coarse-grained chunks: the minimum allocation size if the system virtual memory page size (typically 4K) while allocations occur at page granularity (allocated on 64K boundaries)
- Heap API: the heap API is implemented by the Heap Manager within the system support library _ntdll.dll_; the heap API allows user-mode applications to manage memory allocations and deallocations at a much finer granularity than the virtual API, but does not support all of the options available via the virtual API (such as specifying various memory attributes and protection properties); modern versions of the Heap Manager implement a variety of heap frontend allocators on top of the backend heap allocator implementation
- Local / Global APIs: the local and global memory management APIs are a historical vestige; their use should be avoided in new applications
- C/C++ Runtime: the C/C++ runtime libraries provide memory management routines that make use of the underlying Heap API to implement memory allocation and deallocation; examples of these routines include `malloc()`/`free()` and `operator new`/`operator delete`

One further memory management API provided by the Windows Memory Manager is the system memory pool API. System memory pools, also known as kernel pools, allow the kernel itself as well as kernel drivers to dynamically manage memory allocations and deallocations. There are two (common) kernel memory pool types:
- Paged Pool: allocations from paged pool may be paged out of physical memory by persisting the memory to disk via one of the system page files; memory that is allocated from paged pool may only be accessed at an IRQL < DISPATCH_LEVEL (2) because page faults cannot be handled at an IRQL >= DISPATCH_LEVEL
- Non-Paged Pool: allocations from non-paged pool are always resident in physical memory - they may never be swapped out to disk; allocations from non-paged pool may be accessed at any IRQL

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

### Virtual Address Translation

The virtual address translation process allows the system to translate the virtual addresses upon which programs natively operate to the physical addresses that actually contain the code or data of interest. In the typical implementation of virtual memory, the operating system sets up a hierarchy of _paging structures_ that the hardware may then use during execution to perform address translation. The procedure followed by the hardware during this translation process, enabled by the OS paging structures, is described below for both x86 and x64 systems.

### x86 Address Translation

On x86 systems, virtual addresses are 32-bits in width. To perform the translation from this virtual address to the corresponding physical address, the following procedure is followed:
- The processor locates the base of the _page directory_ by way of the current value stored in the `CR3` model-specific register; this value (the _page directory base pointer_) is persisted across context switches via storage within the `KPROCESS` structure
- The page directory is an array of 1024 4-byte (32-bit) _page directory entries_; the most-significant 10 bits of the virtual address are used as an index into the page directory to locate the page directory entry of interest
- The page directory entry located in the previous step contains the physical address (in the form of a _page frame number_ or PFN) of a _page table_; the page table is structured identically to the page directory apart from the fact that there may be many page tables for a single process while there is only ever a single page directory for each process
- The next-most-significant 10 bits of the virtual address are then used as in index into the selected page table to locate a _page table entry_; this page table entry then points to the start of a page in physical memory
- The final 12 bits of the virtual address are then used as an index into the physical page selected by the page table entry to locate the specific byte referred to by the virtual address

This completes the address translation process on x86 systems. Note that the above procedure applies to x86 address translations on non-PAE (Physical Address Extensions) systems. Under PAE, the translation process is similar, except it introduces an additional layer of tables and the entries in each table are 64-bits in width rather than 32-bits.

### x64 Address Translation

On x64 systems, virtual addresses are 64-bits in width. To perform the translation from this virtual address to the corresponding physical address, the following procedure is followed:
- The processor locates the base of the _page map level 4_ by way of the current value stored in the `CR3` model-specific register; this value (the _page map base pointer_) is persisted across context switches via storage within the `KPROCESS` structure
- The page map level 4 is an array of 512 8-byte (64-bit) entries; each of these entries point to the physical address of a page directory pointers table
- The most-significant 9 bits of the virtual address are used as an index into the page map level 4 to select the physical address of the base of some page directory pointers table
- The next-most-significant 9 bits of the virtual address are used as an index into the page directory pointers table to select a page directory pointer; this page directory pointer yields the physical address of the base of a page directory
- The next-most-significant 9 bits of the virtual address are used as an index into the page directory to select a page directory entry; this page directory entry yields the physical base address of a page table structure
- The next-most-significant 9 bits of the virtual address are used as an index into the page table to select a page table entry; this page table entry yields the physical address of the base of the page in memory where the desired memory resides
- Finally, the least-significant 12 bits of the virtual address are used as an index into the physical memory page to locate the specific byte referred to by the virtual address

This completes the address translation process on x86 systems. 

### Virtual Memory Translation Entries

The table structures utilized in the virtual address translation process are composed of an array of entries that contain physical address pointers (in the form of page frame numbers) to the next lower-level translation structure. However, the paging structures are set up in such a way that only a portion of the bits that are present in the entry are needed to locate the PFN. The remainder of the bits in the entry represent a variety of flags that determine the properties of all memory addresses "downstream" from that entries. These flags include the following:
- No Execute: denotes that the page is non-executable
- Valid: indicates that the entry represents a valid address translation
- Dirty: indicates that the page has been written and has not yet been written back to the page file
- Global: the translation provided by this entry is valid in all process contexts; this entry is not affected by a TLB flush
- etc. 

### References

- _Windows Internals, 7th Edition Part 1_ Pages 301-482
