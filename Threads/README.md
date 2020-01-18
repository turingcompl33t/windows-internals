## Windows Threads

### Thread Attributes: User Space

### Thread Attributes: Kernel Space

### Thread Priorities

The priority of a thread is the key determinant in the scheduling algorithm used by the Windows dispatcher to determine which thread should be run next. Thread priority is expressed relative to the process priority class of the process in which the thread executes. 

Thread priority levels are divided into two distinct ranges:

- Sixteen realtime levels (16-31)
- Sixteen dynamic levels (0-15) where the 0 level is reserved for the zero-page thread

Process priority classes are organized as follows. The number in parentheses indicates the internal `PROCESS_PRIORITY_CLASS` index recognized by the kernel, and the number after the arrow represents the base priority value for that process priority class.

- Real-Time (4) -> 24
- High (3) -> 13
- Above Normal (6) -> 10
- Normal (2) -> 8
- Below Normal (5) -> 6
- Idle (1) -> 4

Thread priorities are then expressed in terms of deltas relative to the process priority class:

- Time Critical (15)
- Highest (2)
- Above Normal (1)
- Normal (0)
- Below Normal (-1)
- Lowest (-2)
- Idle (-15)

Any application may alter the priority of its threads within the dynamic range. In order to enter the real-time range, the process token must contain the `SeIncreaseBasePriorityPrivilege` privilege.

### Thread States

Threads may be in one of the following states at any one time.

- _Ready_: waiting to execute, will be considered by the dispatcher 
- _Deferred Ready_: threads selected to run on a specific processor that have not yet started running there
- _Standby_: the thread scheduled to run next on a particular processor; may be preempted out of this position
- _Running_: executing on a processor
- _Waiting_: the thread is waiting for some event to occur e.g. waiting on some synchronization object
- _Transition_: ready for execution but kernel stack is paged out
- _Terminated_: thread is finished executing, has not yet been destroyed
- _Initialized_: used internally during thread creation

### Thread Local Storage

Thread local storage (TLS) is one method by which a programmer may ensure that program threads access memory that is independent of and protected from other threads within the same process. TLS essentially gives each thread in a process an isolated array of pointers that may be used for arbitrary data management.

Initially, no TLS indexes are allocated. Any thread within a process may allocate and deallocate TLS indices at any time using the following APIs:

- `TlsAlloc()`
- `TlsFree()`

This API obviously presents the potential for disaster: any thread may call `TlsAlloc()` and `TlsFree()`, meaning that a single thread may corrupt the TLS storage region for all other threads running in the process. For this reason, it is common practice to have a single thread (perhaps the initial program thread)perform all TLS allocations and deallocations.

A thread may interact with its assigned TLS array using the following APIs:

- `TlsGetValue()`
- `TlsSetValue()`

A common use case for TLS is in building thread-safe (fully-reentrant) DLLs. The thread can essentially use TLS to communicate arbitrary data with a function exported by a DLL, that is specific to the thread.

### Threads and the C Runtime Library (CRT)

The common Windows thread APIs discussed thus far actually present a major limitation: they are not guaranteed to produce thread-safe behavior when combined with routines provided by the C Runtime Library (CRT). The `CreateThread()` and `ExitThread()` functions do not interact properly with CRT functions. For this reason, Microsoft C (the Visual C / Visual C++ library) provides wrapper functions that handle thread creation and thread exit safely:

- `_beginthreadex()`
- `_endthreadex()`

There are also non-extended versions of these functions. Do not use them.

The functions provided by the CRT eventually call `CreateThread()` / `ExitThread()` behind the scenes, but in addition to this they also perform per-thread bookkeeping necessary to ensure that CRT calls within the thread operate correctly.

Obviously, the CRT is nearly ubiquitous in C/C++ code. However, there are examples of correct multithreaded programs that make use of `CreateThread()` and `ExitThread()` by avoiding calls into the CRT. Astounding.

### Thread APIs

Get and set the process priority class.

- `GetPriorityClass()`
- `SetPriorityClass()`

Get and set the thread priority level.

- `GetThreadPriority()`
- `SetThreadPriority()`

### References 

- _Windows System Programming, 4th Edition_ Pages 223-256
- _Windows Internals, 7th Edition Part 1_ Pages 193-300