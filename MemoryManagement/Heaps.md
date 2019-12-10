## Windows Heap Management

As is explored in the main document in this directory, the Windows virtual memory management APIs such as `VirtualAlloc()` operate at a fixed granularity of 64KB. Obviously, for most allocations performed by user mode applications, this is level of granularity is far from ideal. Rather than force application developers to design and implement their own memory management frontends to manage smaller allocations within the large blocks allocated by the virtual memory APIs, Windows provides a heap manager implementation to perform exactly this task.

The Windows heap manager implementation is utilized both by user mode applications as well as by kernel components. User mode APIs exported by subsystem DLLs call into _ntdll.dll_ in order to access the services of the heap manager, while kernel components call into various executive components. Native heap management functions are prefixed with the _Rtl_ prefix - Runtime Library.

The allocation granularity for the heap manager is 8 bytes on 32-bit platforms and 16 bytes on 64-bit platforms.

### Heap Management Basics

Desktop applications are created with a default heap: the default process heap.

UWP applications are created with three (3) default heaps:

- The default process heap
- The "port heap" used for sharing large memory allocations with the subsystem environment process
- A heap utilized by the C/C++ runtime library memory allocation routines (`malloc()`, `free()`, etc.)

### Heap Types

Beginning with Windows 10, the heap manager implementation utilized by a process may vary.

**NT Heap**

The NT heap was the sole heap implementation up until Windows 10. Today, the NT heap is utilized by default by all applications other than UWP apps and certain system processes.

The NT heap may host an optional frontend layer called the low-fragmentation heap (LFH). The LFH reduces internal fragmentation by allocating memory in fix-sized "buckets" as described in the table below.

| Buckets | Granularity |  Range (bytes) |
|---------|-------------|----------------|
| 1-32    | 8           | 1-256          |
| 33-48   | 16          | 257-512        |
| 49-64   | 32          | 512-1,024      |
| 65-80   | 64          | 1,025-2,048    |
| 81-96   | 128         | 2,049-4,096    |
| 97-112  | 256         | 4,097-8,192    |
| 113-128 | 512         | 8,193-16,384   |

**Segment Heap**

Windows 10 introduced the segment heap, which is essentially a "heap implementation multiplexer." The essense of the segment heap is that the allocation strategy utilized by the heap manager depends upon the size of the allocation requested:

- For allocations less than 16,368 bytes, the LFH is used to service the request, provided the allocation size is a "popular" one (the segment heap LFH implementation is actually distinct from the LFH frontend for the NT heap, but the idea is the same)
- For allocations less than 128KB, the variable size (VS) allocator is used
- For allocations less than 508KB, the heap backend is used to service the request directly
- For allocations larger than 508KB, the heap manager calls the memory manager directly (e.g. `VirtualAlloc()`)

Another important factor of the segment heap is that it maintains its metadata in a region that is separate from the memory allocations themselves, making heap-based exploits more difficult.

The segment heap is used by default for UWP processes. It is also utilized by the following system processes:

- _csrss.exe_
- _lsass.exe_
- _runtimebroker.exe_
- _services.exe_
- _smss.exe_
- _svchost.exe_

The segment heap may be enabled / disabled for desktop applications by setting an `Image File Execution Options` value of `FrontEndHeapDebugOptions` in the registry.

**Pageheap**

Pageheap is merely one among many of the powerful debugging facilities provides to support heap management and security validation. At a high level, the pageheap allocates even small allocations at the end of memory pages and suffixes the allocation with a guard page (of sorts, really it is just reserved) such that if a buffer overflow occurs in the allocation, it is immediately detected as a corruption. Of course, such an implementation is prohibitively expensive outside of debugging scenarios.

**Fault-Tolerant Heap**

The fault-tolerant heap (FTH) is yet another example of the advanced heap manager facilities added in Windows 10. The FTH, composed of client (mitigation) and server (detection) components, detects applications that frequently experience heap-related errors (e.g. access violations) and transparently inserts a shim that attempts to allow the application to continue operation in the presence of these errors.

### Heap Management API

Common heap management functions include:

- `HeapCreate()`
- `HeapDestroy()`
- `HeapAlloc()`
- `HeapFree()`
- `HeapReAlloc()`
- `HeapLock()`
- `HeapUnlock()`
- `HeapWalk()`
- `HeapGetInformation()`
- `HeapSetInformation()`
- `GetProcessHeap()`
- `GetProcessHeaps()`

Various of these APIs accept the `HEAP_NO_SERIALIZE` flag which allows the heap manager to skip the operations involved in synchronizing access to the heap and thereby improve performance. This is appropriate for, for example, single-threaded applications.

### References

- _Windows Internals, 7th Edition Part 1_ Pages 332-348