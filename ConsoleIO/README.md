## Console IO

The Windows console IO API is relatively similar to its UNIX counterpart.

### Standard Devices

In the same way that UNIX defines three standard devices in terms of the following well known file descriptors

- 0 -> `stdin`
- 1 -> `stdout`
- 2 -> `stderr`

Windows defines three standard devices in terms of `HANDLE`s:

- `STD_INPUT_HANDLE`
- `STD_OUTPUT_HANDLE`
- `STD_ERROR_HANDLE`

### Console Handles

A console is both the source and destination of IO operations for console applications. The connection between consoles and standard devices is as follows:

- The standard input (`STD_INPUT_HANDLE`) for the application refers to the console's input buffer
- The standard ouput (`STD_OUTPUT_HANDLE`) and standard error (`STD_ERROR_HANDLE`) for the application refers to the console's screen buffer

### Console Creation and Inheritance

Some notes on console creation and inheritance:

- By default, a call to `CreateProcess()` from a console application entails that the "child" process inherits the console of its "parent"
- A parent process may specify that a child process should NOT inherit its console by specifying the `CREATE_NEW_CONSOLE` flag during process creation
- A parent process may also specify that a child process started via `CreateProcess()` should not attach to a console via the `DETACHED_PROCESS` flag
- Non-console applications (e.g. GUI applications) do not have a console allocated by default; in this case, the application may call `AllocConsole()` to allocate a new console and attach to it

### Console Close

Like all other handles, console handles are reference counted. A console is destroyed when the last process that is attached to the console terminates or calls `FreeConsole()`.

### Console APIs

Get a handle to one of the three standard devices. The argument `nStdHandle` must be one of 

- `STD_INPUT_HANDLE`
- `STD_OUTPUT_HANDLE`
- `STD_ERROR_HANDLE`

```
HANDLE GetStdHandle(DWORD nStdHandle)
```

This function differs from many other handle-manipulation funtions in that invoking it will NOT create a new handle to the standard device - multiple calls to this function may be made in the same process and the same handle will be shared among all of these "apparent instances."

Redirect standard devices.

```
BOOL SetStdHandle(
    DWORD nStdHandle,
    HANDLE hHandle
    )
```

Manipulate the console mode. The `dwMode` argument specifies flags that control how input or output characters are processed by the console.

```
BOOL SetConsoleMode(
    HANDLE hConsoleHandle,
    DWORD dwMode
    )
```

Read from the process console.

```
BOOL ReadConsole(
    HANDLE hConsoleInput,
    LPVOID lpBuffer,
    DWORD cchToRead,
    LPDWORD lpcchRead,
    LPVOID lpReserved
    )
```

Write to the process console.

```
BOOL WriteConsole(
    HANDLE hConsoleOutput,
    LPVOID lpBuffer,
    DWORD cchToWrite,
    LPDWORD lpcchWritten,
    LPVOID lpReserved
    )
```

Allocate or free the process console. A process may only be attached to a single console at any one time. A console may have many processes attached to it.

```
BOOL FreeConsole(VOID)

BOOL AllocConsole(VOID)
```

Attach to the console of the specified process.

```
BOOL AttachConsole(DWORD dwProcessId)
```

### References

- [MSDN Console Programming Reference](https://docs.microsoft.com/en-us/windows/console/)
- _Windows System Programming, 4th Edition_ Pages 39-41
- _Windows System Programming, 4th Edition_ Pages 51-58

