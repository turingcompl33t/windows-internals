## Asynchronous IO

Why bother with asynchronous IO?

- Even if the load or store targets some location in system memory (rather than in persistent storage) the time required for IO operations still dominates program runtime (often)
- Storage device read and write times are often the single most important factor in limiting program performance
- Performing network IO is often most damaging of all

### Asynchronous IO Models

There are three (3) general models available in Windows for implementing asynchronous IO in an application:

1. Multithreaded IO: the program utilizes multiple threads, each of which performs synchronous IO operations; the IO operations from distinct threads do not block one another
2. Overlapped IO with Waiting: the thread issues an IO operation and continues execution; when the thread requires the result of the operation it waits on an object to retrieve it (file handle or event object)
3. Overlapped IO with Completion Routines: the thread specifies a callback function at the time that it issues the IO operation; the callback is invoked when the IO operation completes; also called "extended IO" because it requires calls to `ReadFileEx()` and `WriteFileEx()`

For the purposes of the remainder of this discussion, model 2 will be referred to as "overlapped IO" while model 3 will be referred to as "extended IO".

### Overlapped IO

How does one retrieve the result of the IO operation when performing overlapped IO? The `GetOverlappedResult()` routine allows the program to do one of two things:

- Wait on the overlapped result (block)
- Poll the status of the overlapped result

This behavior is controlled by the `bWait` boolean parameter.

```
BOOL GetOverlappedResult(
    HANDLE hFile,
    LPOVERLAPPED lpOverlapped,
    LPDWORD lpcbTranser,
    BOOL bWait
    )
```

It is important to note that, because multiple overlapped IO operations targeting the same file may be "in-flight" at one time, the file handle itself is useless in uniquely identifying the IO operation to be queried. This is the ole of the `OVERLAPPED` structure - it uniquely identifies the IO operation of interest.

Overlapped IO operations may be cancelled with the `CancelIOEx()` function, but it is still necessary to wait on the result and clean up before moving on.

### Extended IO with Completion Routines

Extended IO involves setting a completion routine (callback function) at the time the IO operation is initiated that is then called by the OS when the IO operation completes. Note, however, that the thread must enter an alertable wait state in order to receive notification that the IO completed.

Enter an alertable wait state with on the five (5) following functions:

- `WaitForSingleObject()`
- `WaitForMultipleObjects()`
- `SleepEx()`
- `SignalObjectAndWait()`
- `MsgWaitForMultipleObjectsEx()`

Because `ReadFile()` and `WriteFile()` give one no way to specify the completion routine at the time the IO is initiated, the extended functions `ReadFileEx()` and `WriteFileEx()` must be used to perform asynchronous IO under the extended model.

The signature for a completion routine is as follows:

```
VOID WINAPI FileIoCompletionRoutine(
    DWORD dwError,
    DWORD cbTransferred,
    LPOVERLAPPED lpOverlapped
    )
```

As in the case of overlapped IO, the `OVERLAPPED` structure is made accessible to the completion routine in order to allow the routine to uniquely identify the IO operation received.

The completion routine is always executed in the context of the thread that initiated the IO operation. All of the thread's queued completion routines are executed when the thread enters an alertable wait state.

### Waitable Timers

Waitable timers are a form of waitable kernel object. They are a useful way to perform specified tasks periodically or at relative or absolute times.

There are two ways to be notified when a timer is signaled:

- Wait on the timer
- Specify a callback routine

There are two forms of waitable timers:

- _Synchronization Timers_ which automatically reset the signaled state of the timer
- _Manual Reset Timers_ which do not 

Waitable timer APIs include:

- `CreateWaitableTimer()`
- `OpenWaitableTimer()`
- `SetWaitableTimer()`
- `CancelWaitableTimer()`

### IO Completion Ports

IO completion ports combine various aspects of multithreading and asynchronous IO. IO completion ports essentially add an additional layer of indirection to the traditional multithreaded server model in which one thread handles IO operations for one client (represented by e.g. a HANDLE to a network socket). In contrast, IO completion ports allow a thread pool to service a pool of HANDLEs associated with clients. This allows a manageable (read: not ridiculous) number of threads to service a large number of clients without incurring the overhead and performance degradation that might be experienced if a new thread were created for each client.

The traditional use case for IO completion ports is server application. Consider the case when the server must manage IO operations to and from a large number of clients. Creating a separate thread to manage each client connection would prove fatal to application performance.

Create a completion port.

```
HANDLE CreateIoCompletionPort(
    HANDLE FileHandle,
    HANDLE ExistingCompletionPort,
    ULONG_PTR CompletionKey,
    DWORD NumberOfConcurrentThread
    )
```

On the first call to this function, specify `INVALID_HANDLE_VALUE` for the `FileHandle` argument to create a new port. On subsequent calls, pass the HANDLE to the completion port itself (obtained from the first call) along with a file HANDLE to add to the newly created completion port.

Once the port is created, issue IO on the handles associated with the completion port via `ReadFile()`, `WriteFile()`, etc.

A thread waits for completed IO operations by calling the `GetQueuedCompletionStatus()` function.

Likewise, a thread posts a completion event to the completion port via the `PostQueuedCompletionStatus()` function.

### References

- _Windows System Programming, 4th Edition_ Pages 481-517