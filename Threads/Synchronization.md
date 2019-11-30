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

**Critical Sections**

`CRITICAL_SECTION` objects are initialized and deleted but do not have handles associated with them and thus cannot be shared with other processes. `CRITICAL_SECTION` objects may be recursively acquired.

`CRITICAL_SECTION` objects are a userspace construct and are those often more performant than the kernel synchronization objects because there is less overhead involved in acquire and release. `CRITICAL_SECTION` objects may also be tuned with an associated _spin count_ which prevents threads waiting on the `CRITICAL_SECTION` from entering the kernel and blocking - a useful tool for regions with high contention. 

Limitations of `CRITICAL_SECTION` objects include:

- No timeout capability (apart from spinning and manually implementing)
- Inability to signal another thread

Initialize and delete a critical section object. 

```
VOID InitializeCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
    )

VOID DeleteCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection
    )
```

Enter and leave critical sections.

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

**Mutexes**

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

**Semaphores**

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

**Events**

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

### Thread Synchronization Structures

TODO

### References

- _Windows System Programming, 4th Edition_ Pages 259-298

