## Windows Synchronization Cheatsheet (Win32)

The Win32 API offers a formidable number of synchronization primitives. We all need a little help every now and then deciding which one best suits our needs.

### Table of Contents

- [Condition Variable](#condition-variable)
- [Critical Section](#critical-section)
- [Event](#event)
- [One-Time Initialization](#one-time-initialization)
- [Mutex](#mutex)
- [Semaphore](#semaphore)
- [Slim Reader/Writer Lock](#slim-reader-writer-lock)
- [Synchronization Barrier](#synchronization-barrier)
- [Waitable Address](#waitable-address)

### Condition Variable

| Windows Type         | Kernel Object |
|----------------------|---------------|
| `CONDITION_VARIABLE` | No            |

API Functions

- `InitializeConditionVariable()`
- `SleepConditionVariableCS()`
- `SleepConditionVariableSRW()`
- `WakeConditionVariable()`
- `WakeAllConditionVariable()`

### Critical Section

| Windows Type         | Kernel Object |
|----------------------|---------------|
| `CRITICAL_SECTION`   | No            |

API Functions

- `InitializeCriticalSection()`
- `InitializeCriticalSectionAndSpinCount()`
- `EnterCriticalSection()`
- `TryEnterCriticalSection()`
- `LeaveCriticalSection()`
- `DeleteCriticalSection()`

### Event

| Windows Type         | Kernel Object |
|----------------------|---------------|
| `Event` (`HANDLE`)   | Yes           |

API Functions

- `CreateEvent()`
- `OpenEvent()`
- `SetEvent()`
- `PulseEvent()` (use discouraged)
- `ResetEvent()` (manual-reset only)
- `WaitForSingleObject()`
- `WaitForMultipleObjects()`
- `CloseHandle()`

Event Behavior Summary

|                | Auto-Reset                                                                             | Manual-Reset                                                                            |
|----------------|----------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------|
| `SetEvent()`   | One thread released. If none waiting, first to wait released immediately. Event reset. | All currently waiting threads released. Event must be manually reset by `ResetEvent()`. |
| `PulseEvent()` | One thread released. If none waiting, none are released. Event reset.                  | All currently waiting threads released. Event reset.                                    |

### One-Time Initialization

| Windows Type         | Kernel Object |
|----------------------|---------------|
| `INIT_ONCE`          | No            |

Static Initializer: `INIT_ONCE_STATIC_INIT`

API Functions

- `InitOnceInitialize()`
- `InitOnceExecuteOnce()` (synchronous)
- `InitOnceBeginInitialize()` (asynchronous)
- `InitOnceComplete()` (asynchronous)

Callback Signature

```
BOOL CALLBACK InitHandleFunction (
    PINIT_ONCE InitOnce,        
    PVOID      Parameter,            
    PVOID      *lpContext) 
```

### Mutex 

| Windows Type         | Kernel Object |
|----------------------|---------------|
| `Mutant` (`HANDLE`)  | Yes           |

API Functions

- `CreateMutex()`
- `CreateMutexEx()`
- `OpenMutex()`
- `WaitForSingleObject()`
- `WaitForMultipleObjects()`
- `ReleaseMutex()`
- `CloseHandle()`

### Semaphore

| Windows Type           | Kernel Object  |
|------------------------|----------------|
| `Sempahore` (`HANDLE`) | Yes            |

API Functions

- `CreateSemaphore()`
- `WaitForSingleObject()`
- `WaitForMultipleObjects()`
- `ReleaseSemaphore()`
- `CloseHandle()`

### Slim Reader/Writer Lock

| Windows Type         | Kernel Object |
|----------------------|---------------|
| `SRWLOCK`            | No            |

Static Initializer: `SRWLOCK_INIT`

API Functions

- `InitializeSRWLock()`
- `AcquireSRWLockShared()`
- `AcquireSRWLockExclusive()`
- `TryAcquireSRWLockShared()`
- `TryAcquireSRWLockExclusive()`
- `ReleaseSRWLockShared()`
- `ReleaseSRWLockExclusive()`

### Synchronization Barrier

| Windows Type              | Kernel Object |
|---------------------------|---------------|
| `SYNCHRONIZATION_BARRIER` | No            |

API Functions

- `InitializeSynchronizationBarrier()`
- `EnterSynchronizationBarrier()`
- `DeleteSynchronizationBarrier()`

### Waitable Address

| Windows Type         | Kernel Object |
|----------------------|---------------|
| `VOID*`              | No            |

API Functions

- `WaitOnAddress()`
- `WakeByAddressSingle()`
- `WakeByAddressAll()`