## Thread Synchronization

Windows thread synchronization objects, APIs, and practices.

### Some Terminology

_Thread Safety_

- Code is thread safe when multiple threads may execute the code simultaneously without producing undesirable or unexpected results.

_Critical Code Region_

- A region of code that must be executed atomically in order to ensure correct behavior. That is, only a single thread may execute the critical code region at a time - access must be serialized. 

_Invariant_

- A property of an object's state that is guaranteed to be true outside of a critical code region by proper program implementation. 

### `volatile` Storage

The ANSI C `volatile` storage qualifier prevents an optimizing compiler implementation from storing the volatile object in a register, and instead forces the object to be written to memory after each modification and read from memory before each use. The `volatile` qualifier tells the compiler that the object may change at any time.

Note that in the above description, the word "memory" is misleading - because modern memory hierarchies include multiple levels of data cache, the volatile object is actually only guaranteed to be read from and written to the executing processor's L1 cache on each operation.

### Interlocked Functions

Windows provides interlocked functions to perform simple atomic operations. Be wary when using these, however, as they introduce an implicit memory barrier. The interlocked functions are only atomic with respect to the other interlocked functions.

Interlocked functions are implemented in userspace using atomic machine instructions. They are essentially compiler intrinsics.

Examples of interlocked functions include:

```
LONG InterlockedIncrement(
    LONG volatile *Addend
    )

LONG InterlockedDecrement(
    LONG volatile *Addend
    )

LONG InterlockedExchange(
    LONG volatile *Target,
    LONG Value
    )

PVOID InterlockedCompareExchange(
    LONG volatile *Destination,
    LONG Exchange,
    LONG Comparand
    )
```

There are variants of these functions for incrementing and decrementing 64-bit values.

### Thread Synchronization Objects

Windows provides four objects for process and thread synchronization:

- `CRITICAL_SECTION`
- _mutex_
- _semaphore_
- _event_

The latter three in the above list are all kernel objects, while `CRITICAL_SECTION` is not a kernel object.

### Critical Sections

`CRITICAL_SECTION` objects are initialized and deleted but do not have handles associated with them and thus cannot be shared with other processes. `CRITICAL_SECTION` objects may be recursively acquired.

`CRITICAL_SECTION` objects are a userspace construct and are thus often more performant than the kernel synchronization objects because there is less overhead involved in acquire and release. `CRITICAL_SECTION` objects may also be tuned with an associated _spin count_ which prevents threads waiting on the `CRITICAL_SECTION` from entering the kernel and blocking - a useful tool for regions with high contention. 

Limitations of `CRITICAL_SECTION` objects include:

- No timeout capability (apart from spinning and manually implementing)
- The inability to signal another thread

The APIs available for initializing and deleting a critical sections include: 

```
VOID InitializeCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
    )

VOID DeleteCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
    )
```

The APIs available for entering and leaving critical sections include:

```
VOID EnterCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
    )

BOOL TryEnterCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
    )

VOID LeaveCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
    )
```

### Mutexes

Mutexes are synchronization objects that offer additional functionality beyond that provided by `CRITICAL_SECTION`s:

- Abandoned mutexes are signaled, making deadlock avoidance and bug identification easier
- Mutexes support wait timeouts
- Mutexes can be named and thus shared between threads in different processes
- Mutexes support immediate ownership upon creation

These added features come at the cost of performance: `CRITICAL_SECTION`s typically allow faster program execution than mutexes.

Synchronization APIs related to mutexes include:

- `CreateMutex()`
- `OpenMutex()`
- `ReleaseMutex()`
- `WaitForSingleObject()`
- `WaitForMultipleObjects()`

### Semaphores

Semaphores are a slightly more complex synchronization object that maintain an internal count.
- The semaphore is signaled whenever the internal count is greater than 0
- The semaphore is unsignaled when the internal count is 0

Synchronization APIs related to semaphores include:

- `CreateSemaphore()`
- `CreateSemaphoreEx()`
- `OpenSemaphore()`
- `ReleaseSemaphore()`
- `WaitForSingleObject()`
- `WaitForMultipleObjects()`

### Events

Events are synchronization objects that allow multiple threads to be released from a wait simultaneously, under several distinct configuration options. Events may be classified as manual-reset or auto-reset.

The four event usage models are summarized below. Each usage model is applicable in certain situations.

- Auto-Reset
    - `SetEvent()`: Exactly one thread is released; if no threads are waiting at the time the event is signaled, the first thread to wait on the event in the future will be released; the event is automatically reset
    - `PulseEvent()`: Exactly one thread is released; if no threads are waiting on the event that the time the event is signaled, the event is reset and no threads are released
- Manual-Reset
    - `SetEvent()`: All currently waiting threads are released; the event remains signaled
    - `PulseEvent()`: All currently waiting threads are released; the event is reset

Synchronization APIs related to events include:

- `CreateEvent()`
- `CreateEventEx()`
- `OpenEvent()`
- `SetEvent()`
- `ResetEvent()`
- `PulseEvent()`

### Comparing Locking Perforance: `CRITICAL_SECTION` vs Mutex

As noted above, using a critical section to enforce mutual exclusion often introduces less overhead during locking operations relative to a mutex because the mutex is a kernel object. But how significant is this disparity? Empirical tests confirm: quite significant.

Another important consideration is the fact that mutex performance actually degrades with increased compute resources (processor count, clock speed) while the performance of critical sections improves. Thus, critical sections may be said to be a far more scalable locking solution than mutexes.

### Aside: _False Sharing_

On multiprocessor systems, a phenomenon of _false sharing_ may occur when the data accessed by independent threads resides on the same cache line. That is, even though the data is local and private to each thread, the fact that the data resides on the same cache line introduces a false sharing dependency between the two sets. Such false sharing dependencies can needlessly degrade program performance as cache coherency must be maintained.

Avoid false sharing dependencies by properly aligning data structures containing thread arguments on cache line boundaries.

### `CRITICAL_SECTION`: Under the Hood

How does the `CRITICAL_SECTION` achieve such superior performance relative to mutexes?
- `EnterCriticalSection()` does an atomic bit test and set to "acquire" access to the critical section
- If the critical section is locked when this call is made, one of two things happen:
    - On a single processor system, the thread waits on the critical section with `WaitForSingleObject()`, this is obviously slow
    - On a multiprocessor system, the thread may enter a tight loop and spin on the critical section lock bit for a specified number of iterations, thus never yielding the processor

The reason for this distinction is that on a single processor system, there can be no other thread active to reset the critical section lock bit, so the processor must be yielded in order to allow some other thread to execute and (hopefully) exit the critical section.

How do we set the critical section spin count?

- At initialization time with `InitializeCriticalSectionAndSpinCount()`
- After initialization with `SetCriticalSectionSpinCount()`

### NT6 Slim Reader/Writer Locks

Windows NT6 added a new synchronization primitive: the slim reader/writer lock. As the name implies, these locks may be acquired in one of two modes:

- exclusive mode (write)
- shared mode (read)

SRW locks differ from critical sections in the following ways:

- Small, the size of a pointer
- Cannot be recursively acquired
- The spin count cannot be configured

The API for working with SRW locks is as follows:

- `InitializeSRWLock()`
- `AcquireSRWLockShared()`
- `ReleaseSRWLockShared()`
- `AcquiredSRWLockExclusive()`
- `ReleaseSRWLockExclusive()`

### NT6 Condition Variables

Windows NT6 also added support for another useful synchronization object: the condition variable. Like `CRITICAL_SECTION` objects, condition variables (typed as `CONDITION_VARIABLE`) are user objects rather than kernel objects, implying that they present the same performance benefits over kernel synchronization objects like mutexes and events (which, as it happens, are typically combined to implement the "condition variable model").

Condition variables may be utilized with either critical sections or slim reader/writer locks as the underlying mutual exclusion primitive.

The API for working with Windows condition variables is as follows:

- `InitializeConditionVariable()`
- `SleepConditionVariableCS()`
- `SleepConditionVariableSRW()`
- `WakeConditionVariable()`
- `WakeAllConditionVariable()`

### Managing Thread Contention

On larger systems in which a large number of threads may be created to service asynchronous tasks, enforcing mutual exclusion may become prohibitively expensive as the large number of threads impose an intolerable level of contention on the mutual exclusion primitive (e.g. mutex). There are various ways of managing such situations in mulithreaded environments to improve performance.

One such method is _semaphore throttling_ in which a semaphore is used to limit the number of threads active at any one time.

IO completion ports are another method for managing contention by limiting the number of threads used to execute tasks.

A final option is to use the built-in Windows thread pool support. This allows the programmer to defer the work of determining the optimal number of threads needed to service tasks to the Windows executive. Each process has a dedicated thread pool, but it is also possible to create additional thread pools within a process. The thread pool API was significantly improved in NT6.

### Processor Affinity

Windows maintains the notion of three distinct levels of affinity mask in the system:

- The system affinity mask describes all of the processors available to the system
- The process affinity mask describes all of the processors on which the threads of a process may execute; by default it is equivalent to the system affinity mask
- The thread affinity mask describes all of the processors on which the thread may execute; the thread affinity mask must be a subset of the process affinity mask; by default it is equivalent to the process affinity mask

The APIs available for working with affinity masks include:

- `GetSystemInfo()`
- `GetProcessAffinityMask()`
- `SetProcessAffinityMask()`
- `SetThreadAffinityMask()`
- `SetThreadIdealProcessor()`

### Asynchronous Procedure Calls (APC)

The synchronization mechanisms described thus far do not provide a general solution to the problem of allowing one thread to directly signal another thread. APCs fill this functionality gap by allowing one thread to queue an APC directly to another and therein specify some action that the target thread should perform.

One thread queues work to another thread with the `QueueUserAPC()` function.

The target thread executes any queued APCs by entering an alterable wait state, accomplished by making one of the following function calls:

- `SleepEx()`
- `WaitForSingleObjectEx()`
- `WaitForMultipleObjectsEx()`
- `SignalObjectAndWait()`

In general, APCs are useful primitives for implementing _synchronous thread cancellation_. The cancellation is synchronous in the sense that the target thread must cooperate in the cancellation by entering an alertable wait state, at which point it executes the APC routine. Windows does not support asynchronous thread cancellation.

### Waiting on a Specific Memory Location: `WaitOnAddress()`

See the [Cheatsheet](./Cheatsheet.md)

### Synchronization Barriers

See the [Cheatsheet](./Cheatsheet.md)

### References

- _Windows System Programming, 4th Edition_ Pages 259-298
- _Windows 10 System Programming_ Chapter 7
- `WaitOnAddress()` [Blog Post](https://devblogs.microsoft.com/oldnewthing/20160823-00/?p=94145) 
- [Windows with C++: The Evolution of Synchronization with Windows and C++](https://docs.microsoft.com/en-us/archive/msdn-magazine/2012/november/windows-with-c-the-evolution-of-synchronization-in-windows-and-c)
