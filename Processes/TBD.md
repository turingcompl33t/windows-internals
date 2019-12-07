### Non-Standard Process Types

- Protected Processes: Introduced in Windows Vista to support Digital Rights Management (DRM). The distinguishing feature of a protected process is the fact that its entire address space is locked down from inspection by other processes, including those running under an administrator token.
- UWP Processes: Introduced in Windows 8. The distinguishing feature of UWP processes is that they host the Windows Runtime (WinRT) and run with AppContainer application sandboxing. They are typically used to host Modern Windows Applications (Windows Store apps).
- Protected Process Light (PPL): A generalization of protected processes. PPLs introduced the ability to configure each process with different levels of protection via signers and began allowing third party applications to run in the context of PPLs.
- Minimal Processes: Available in Windows 10 1607. A minimal processes is essentially an empty virtual address space.
- Pico Process: Available in Windows 10 1607. A pico process is just a minimal process that has an associated pico provider (kernel driver) to handle its system calls. WSL 1 is implemented via pico processes and providers.

### Process States

It often makes more sense to talk about _thread states_ rather than _process states_ because a process is really just a management container for threads which actually execute. Nevertheless, Task Manager reports processes in one of three (3) states:

- Running
- Suspended
- Not Responding

The precise meaning of each of these states depends upon the particular process type in question.

### Aside: Viewing Imports and API Sets

If we use a tool like _dumpbin.exe_ to view the imports of a particular executable, some of the entries may appear to be strange names that do no correspond to an actual file on disk. These _API sets_ are not proper DLLs, but rather a layer of indirection between a set of functions and the library (DLL) that actually implements them. The mapping from API set to implementation may differ from image to image (which allows the same API set to map to different implementations for different images, increasing flexibility) and is stored in the process PEB.

Use the _ApiSetMap.exe_ tool to dump API set to implementation mappings.

### Aside: Process Loader Search Path

The search path below is utilized for various process-related APIs including DLL loading and calls to `CreateProcess()`.

1. The directory in which calling program's executable is located
2. The current directory of the process
3. The _System_ directory
4. The _Windows_ directory
5. Directories included in the `PATH` environment variable

### Process Startup

There are four (4) valid application entry points in C/C++ applications for Windows, and four (4) distinct associated C/C++ runtime entry points that are invoked by the system prior to transferring control to the application's entry point. The mapping is shown in the table below.

- `main()` <- `mainCRTStartup()`: used by console applications using ASCII characters
- `wmain()` <- `wmainCRTStartup()`: used by console applications using Unicode characters
- `WinMain()` <- `WinMainCRTStartup()`: used by GUI applications using ASCII characters
- `wWinMain()` <- `wWinMainCRTStartup()`: used by GUI applications using Unicode characters

### Command Line and Environment Variables

Useful APIs for working with environment variables include:

- `GetEnvironmentVariable()`
- `SetEnvironmentVariable()`
- `ExpandEnvironmentStrings()`

Useful APIs for working with the process command line include:

- `GetCommandLine()`
- `CommandLineToArgv()`
- `CommandLineToArgvW()`

### Aside: `CreateProcess()` and Failure

A call to `CreateProcess()` will return successfully within the context of the parent (creating) process once system (kernel) initialization of the process has completed. However, it is still possible for the user space initialization of the new process to fail after this report has been made. 

It may be possible for this distinction to lead to synchronization or dependency issues when a child process is depended upon by a parent e.g. via `WaitForSingleObject()` on the child process handle.

### Aside: Process Current Directory

The current directory of a process defines the location at which various filesystem searches originating from within the process will begin. For instance, if a full file path is excluded from a call to `CreateFile()`, the process current directory is consulted and the current directory is searched for the requested file (or a new one is created, according to the creation disposition flags).

- `GetCurrentDirectory()`
- `SetCurrentDirectory()`

A related topic is the _process drive directory_ which entails determining what is meant by paths like _C:file.txt_. The system handles this by tracking the current directory for each drive via environment variables.

One may utilize the `GetFullPathName()` function to query the current drive directory for any drive by specifying `<DRIVE_LETTER>:` as the first argument to the call.

### Aside: Window Stations and Desktops

A _Window Station_ is kernel object that is part of a session. In turn, a _Desktop_ is an object associated with a window station that contains things like windows, menus, and hooks.

A single window station may have multiple desktops.

The default window station for the interactive session is _WinSta0_.

Is is only possible to have a single window station associated with a session?

### Aside: Specifying Process and Thread Attributes

The `STARTUPINFO` and `STARTUPINFOEX` structures are used to pass startup information and attributes to child processes created with the `CreateProcess()` API.

The `STARTUPINFOEX` structure extends the `STARTUPINFO` structure by adding a `PROC_THREAD_ATTRIBUTE_LIST`. This attribute list allows for the addition of an unlimited number of attributes.

```
typedef struct _STARTUPINFOEX
{
    STARTUP_INFO StartupInfo;
    PPROC_THREAD_ATTRIBUTE_LIST pAttributeList;
} STARTUPINFOEX, *LPSTARTUPINFOEX;
```

1. Initialize the attribute list with `InitializeProcThreadAttributeList()`
2. Add attributes with `UpdateProcThreadAttribute()`
3. Set the `pAttributeList` member of the `STARTUPINFOEX` structure to point to the attribute list
4. Call `CreateProcess()` with the `EXTENDED_STARTUPINFO_PRESENT` flag set, and the `STARTUPINFOEX` structure passed to the invocation
5. Destroy the attribute list with `DeleteProcThreadAttributeList()` and deallocate the memory utilized for the attribute list
