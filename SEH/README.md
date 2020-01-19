## Structured Exception Handling (SEH)

Structured Exception Handling (SEH) is a mechanism provided by the Windows operating system for handling exceptional control flow situations.

### SEH from C/C++: The Basics

As the statement above suggests, SEH is an OS facility provided by Windows. From there, compilers such as MSVC provide a simplified syntax through which developers may work with this OS facility via extensions to the C/C++ languages.

**Filter Expressions**

Filter expressions are the expressions used to determine the behavior of an associated exception (`__except()`) block.

**Leaving a `__try` Block**

The `__try` block is exited under the following conditions:

- Reaching the end of the `__try` block and "falling through" to the associated termination handler
- Execution of one of the following statements:
    - `return`
    - `break`
    - `goto`
    - `longjmp`
    - `continue`
    - `__leave`
- An exception is raised

**Abnormal Termination**

A `__try` block is said to have been "abnormally terminated" in the event that the block is exited in any manner other than "falling through" to the termination handler. 

Note that the `__leave` statement above is an exception (pun intended?) to this rule in that the effect of the `__leave` statement is to effectively jump to the end of the `__try` block and proceed to fall through to the termination handler. Thus, one may prematurely exit a `__try` block without incurring abnormal termination via the `__leave` statement.

One may test the termination condition of the `__try` block by utilizing the following function in the body of the termination handler:

```
BOOL AbnormalTermination(VOID)
```

**Combining Exception and Termination Blocks**

The following program structure fails to compile:

```
__try
{
    ...
}
__except (FilterExpression(...))
{
    ...
}
__finally
{
    ...
}
```

This fails to compile because a `__try` block must have one of an associated `__except` block or `__finally` block, but it cannot have both. However, one may achieve a functionally-equivalent result by nesting blocks within one another.

### SEH Internals

The signature for an SEH exception handler (from _excpt.h_, found in the kernel mode headers of the SDK):

```
_CRTIMP EXCEPTION_DISPOSITION 
__cdecl __C_specific_handler (
    _In_ struct _EXCEPTION_RECORD * ExceptionRecord,
    _In_ void * EstablisherFrame,
    _Inout_ struct _CONTEXT * ContextRecord,
    _Inout_ struct _DISPATCHER_CONTEXT * DispatcherContext
);
```

The definition of the `EXCEPTION_RECORD` structure (from _winnt.h_):

```
typedef struct _EXCEPTION_RECORD {
    DWORD    ExceptionCode;
    DWORD ExceptionFlags;
    struct _EXCEPTION_RECORD *ExceptionRecord;
    PVOID ExceptionAddress;
    DWORD NumberParameters;
    ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
    } EXCEPTION_RECORD;
```

As an aside, the `typedef` for the `EXCEPTION_ROUTINE` callback function type is also defined in _winnt.h_, along with the `CONTEXT` structure which represents the current thread context (e.g. register state) at the time an exception occurs.

### Console Control Handlers

Console control handlers are a mechanism by which processes may register callback functions that certain special, console-related inputs are recieved by one of the threads in the process. The inputs the process may register to respond to are identified by the following five `DWORD` values:

- `CTRL_C_EVENT`: indicates that the CTRL-C key sequence was entered from the keyboard
- `CTL_CLOSE_EVENT`: indicates that the console window is being closes
- `CTR_BREAK_EVENT`: indicates that the CTRL-Break signal was received 
- `CTRL_LOGOFF_EVENT`: indicates that the user for the session in which the application is running is logging off
- `CTRL_SHUTDOWN_EVENT`: indicates that the operating system is shutting down

One may set and remove arbitrary console control handlers in an application through use of the `SetConsoleControlHandler()` function. The function takes a callback function as an argument that specifies the handler callback that should be added or removed:

```
BOOL HandlerCallback(DWORD)
```

The argument received by the callback function indicates the console control event that was received by the process - this value may be one of the five values enumerated above.

As alluded to above, the console control handler signal is delivered on a process-basis, rather than a thread basis. A new thread is instantiated to execute any handler routines registered by threads within the process.

### References

- _Windows System Programming, 4th Edition_ Pages 101-124
- [Unwinding the Stack: Exploring How C++ Exceptions Work on Windows (Video)](https://www.youtube.com/watch?v=COEv2kq_Ht8&t=357s)
- [Unwinding the Stack: Exploring How C++ Exceptions Work on Windows (Slides)](https://github.com/tpn/pdfs/blob/master/2018%20CppCon%20Unwinding%20the%20Stack%20-%20Exploring%20how%20C%2B%2B%20Exceptions%20work%20on%20Windows%20-%20James%20McNellis.pdf)
- [A Crash Course on the Depths of Win32 Structured Exception Handling](http://bytepointer.com/resources/pietrek_crash_course_depths_of_win32_seh.htm)
- [Exception Behavior - x64 Structured Exception Handling](https://www.osronline.com/article.cfm%5Earticle=469.htm)