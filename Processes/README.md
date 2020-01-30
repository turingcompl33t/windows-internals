## Windows Processes

### Process and Thread Overview

**Process Components**

- One or more threads
- A private virtual address space (excluding shared memory regions)
- One or more code segments
- One or more data segments
- Environment strings
- The process heap
- Resources (e.g. handles and other heaps)

**Thread Components**

- Thread stack
- Thread local storage (TLS)
- An argument on the stack, passed by creating thread
- Context structure with machine register values (maintained by kernel)

**Non-Standard Process Types**

- Protected Processes: Introduced in Windows Vista to support Digital Rights Management (DRM). The distinguishing feature of a protected process is the fact that its entire address space is locked down from inspection by other processes, including those running under an administrator token.
- UWP Processes: Introduced in Windows 8. The distinguishing feature of UWP processes is that they host the Windows Runtime (WinRT) and run with AppContainer application sandboxing. They are typically used to host Modern Windows Applications (Windows Store apps).
- Protected Process Light (PPL): A generalization of protected processes. PPLs introduced the ability to configure each process with different levels of protection via signers and began allowing third party applications to run in the context of PPLs.
- Minimal Processes: Available in Windows 10 1607. A minimal processes is essentially an empty virtual address space.
- Pico Processes: Available in Windows 10 1607. A pico process is just a minimal process that has an associated pico provider (kernel driver) to handle its system calls. WSL 1 is implemented via pico processes and providers.

### Basic Process Management

**Process States**

It often makes more sense to talk about _thread states_ rather than _process states_ because a process is really just a management container for threads which actually execute. Nevertheless, Task Manager reports processes in one of three (3) states:

- Running
- Suspended
- Not Responding

The precise meaning of each of these states depends upon the particular process type in question.

**Creating Processes**

While is it often convenient to think of a parent-child relationship between processes, Windows does not actually recognize such a relationship. However, it does appear to have meaning in several rare cases. 

What is the search path used for the command line specified in a call to `CreateProcess()`?

- The directory of the current process's image
- The current directory (working directory?)
- The Windows system directory (`GetSystemDirectory()`)
- The Windows directory (`GetWindowsDirectory()`)
- The directories specified in the environment `PATH`

**Inheriting Handles**

- Set the `bInheritHandles` flag in the `CreateProcess()` call
- Make the handle itself inheritable; this is done by using the `SECURITY_ATTRIBUTES` structure at the time the handle is acquired
- Communicate the inherited handle to the child
    - Use an IPC mechanism
    - Assign the inherited handle to one of the child's standard IO handles

**Process Identity**

The following APIs are useful for querying process identity:

- `GetCurrentProcess()`
- `GetCurrentProcessId()`
- `OpenProcess()`
- `GetModuleFileName()`
- `GetModuleFileNameEx()`

**Process Environment**

Use the `GetEnvironmentVariable()` and `SetEnvironmentVariable()` functions to get and set the environment variables for the current process.

**Process Signaling**

Windows does not support _signals_ in the same way that UNIX operating systems do. However, one method by which a process may "signal" another is through the use of a console control handler. Furthermore, one process may specify itself as the root of a process group and thereby signal all other processes in the group with the same console control event. This, of course, assumes that the processes share the same console.

Generate a console control event manually with `GenerateConsoleCtrlEvent()`.

**Process Termination**

One may exit the current process with `ExitProcess()`. Termination handlers will be ignored.

Another process may query the exit code of another process using `GetExitCodeProcess()`.

One process may terminate another using `TerminateProcess()`.

One process may wait on another process or a collection of processes to exit using `WaitForSingleObject()` or `WaitForMultipleObjects()`, provided the waiting process has `SYNCHRONIZE` access to the target.

**Process Execution Time**

One process may query the execution time of another process using the `GetProcessTimes()` function.

**Viewing Imports and API Sets**

If we use a tool like _dumpbin.exe_ to view the imports of a particular executable, some of the entries may appear to be strange names that do no correspond to an actual file on disk. These _API sets_ are not proper DLLs, but rather a layer of indirection between a set of functions and the library (DLL) that actually implements them. The mapping from API set to implementation may differ from image to image (which allows the same API set to map to different implementations for different images, increasing flexibility) and is stored in the process PEB.

Use the _ApiSetMap.exe_ tool to dump API set to implementation mappings.

**Jobs**

Processes may be controlled in a centralized manner through the use of job objects.

Job management APIs include the following:

- `CreateJobObject()`
- `OpenJobObject()`
- `AssignProcessToJobObject()`
- `QueryJobInformationObject()`
- `SetInformationJobObject()`

### Windows Process Subsystems

Each executable image is bound to one and only one _environment subsystem_. The role of an environment subsystem is to expose some subset of Windows executive services to an application, implying that applications built upon one subsystem may have distinct "capabilities" from those of an application built on top of another subsystem.

The subsystem type code is included in the executable image for a program. The subsystem code may be manually specified via the `/SUBSYSTEM` linker option.

The most common subsystem (are any others still valid?) is the Windows subsystem.

An aside: tools like Dependency Walker may display the subsystem for a process as "GUI" or "Console", implying that separate subsystems exist for GUI and console applications. This implication is incorrect. Indeed, a GUI application may easily allocate and attach to a console through the console API functions exposed by the Windows subsystem. The upside: there is only ONE Windows subsystem.

An environment subsystem is defined primarily by two things:

- The environment subsystem DLLs that expose the functionality of the subsystem
- The environment subsystem process that runs in user mode and is responsible for managing and responding to requests from all processes running on top of the subsystem for which it is responsible

When a program makes a call into one of the subsystem DLLs, three things may occur:

- The subsystem DLL routine handles the call
- The subsystem DLL routine calls into the kernel to make use of some system service exposed by the executive
- The subsystem DLL routine calls into the environment subsystem process to service the request

These options are not mutually exclusive; a single function invocation may involve more than one of these options.

Certain "special" processes do not rely on any subsystem. Such processes are designated with the subsystem type _native_. Native applications run directly on top of _ntdll.dll_. An example of such a process is the Session Manager Subsystem, _smss.exe_.

**Subsystem DLLs**

The following DLLs expose the functionality of the Windows subsystem:

- _kernel32.dll_
- _advapi32.dll_
- _user32.dll_
- _gdi32.dll_

**Environment Subsystem Processes**

The Windows environment subsystem process is the Client Server Runtime Subsystem, or _csrss.exe_.

**Aside: Console Hosts**

The console host architecture has changed multiple times throughout the recent history of Windows. 

In its current instantiation, the console host process, _conhost.exe_, is spawned during the image loading procedure for any console-based application (and it is spawned on-demand by a GUI application when it calls `AllocConsole()`). The console application then communicates with _conhost.exe_ via the console driver _condrv.sys_. In this model, the console host process functions as a server while the console application that spawned it operates as a client.

### Process Lifecycle: Process Creation

The high level flow of the `CreateProcess*()` functions is as follows:

1. Validate parameters, convert Windows subsystem-specific flags and attributes to their native counterparts
2. Open the image file to be executed within the new process
3. Create the executive process object
4. Create the initial thread (e.g. stack, executive thread object)
5. Perform post-creation, Windows subsystem-specific initialization
6. Start execution of the initial thread (unless `CREATE_SUSPENDED`)
7. In the new process / thread context, perform user space initialization (e.g. load DLLs) and begin execution at the image entry point

### Process Startup

There are four (4) valid application entry points in C/C++ applications for Windows, and four (4) distinct associated C/C++ runtime entry points that are invoked by the system prior to transferring control to the application's entry point. The mapping is shown in the table below.

- `main()` <- `mainCRTStartup()`: used by console applications using ASCII characters
- `wmain()` <- `wmainCRTStartup()`: used by console applications using Unicode characters
- `WinMain()` <- `WinMainCRTStartup()`: used by GUI applications using ASCII characters
- `wWinMain()` <- `wWinMainCRTStartup()`: used by GUI applications using Unicode characters

### Process Execution Environment

**Command Line and Environment Variables**

Useful APIs for working with environment variables include:

- `GetEnvironmentVariable()`
- `SetEnvironmentVariable()`
- `ExpandEnvironmentStrings()`

Useful APIs for working with the process command line include:

- `GetCommandLine()`
- `CommandLineToArgv()`
- `CommandLineToArgvW()`

**`CreateProcess()` and Failure**

A call to `CreateProcess()` will return successfully within the context of the parent (creating) process once system (kernel) initialization of the process has completed. However, it is still possible for the user space initialization of the new process to fail after this report has been made. 

It may be possible for this distinction to lead to synchronization or dependency issues when a child process is depended upon by a parent e.g. via `WaitForSingleObject()` on the child process handle.

**Process Current Directory**

The current directory of a process defines the location at which various filesystem searches originating from within the process will begin. For instance, if a full file path is excluded from a call to `CreateFile()`, the process current directory is consulted and the current directory is searched for the requested file (or a new one is created, according to the creation disposition flags).

- `GetCurrentDirectory()`
- `SetCurrentDirectory()`

A related topic is the _process drive directory_ which entails determining what is meant by paths like _C:file.txt_. The system handles this by tracking the current directory for each drive via environment variables.

One may utilize the `GetFullPathName()` function to query the current drive directory for any drive by specifying `<DRIVE_LETTER>:` as the first argument to the call.

**Window Stations and Desktops**

A _Window Station_ is kernel object that is part of a session. In turn, a _Desktop_ is an object associated with a window station that contains things like windows, menus, and hooks.

A single window station may have multiple desktops.

The default window station for the interactive session is _WinSta0_.

Is is only possible to have a single window station associated with a session?

**Specifying Process and Thread Attributes**

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

### Process Lifecycle: Process Termination

A process may terminate for one of three reasons:

- All of the threads executing within the process terminate
- One of the process threads calls `ExitProcess()`
- The process is terminated via a call to `TerminateProcess()`

A process exits gracefully by calling `ExitProcess()`. Most of the time, this function is called implicitly by the process startup code (the runtime) when the process's initial thread returns from the main function. Here, graceful exit entails the ability of DLLs loaded in the address space of the process to perform cleanup operations (via `DLL_PROCESS_DETACH` handlers in `DllMain()`) and that proper respect is paid to SEH termination handlers.

In contrast, a call to `TerminateProcess()` does not guarantee such a graceful exit.

### Process Structures in User Space

The process environment block (`PEB`) is the primary process-related data structure that exists in user space. The `PEB` contains information needed by the image loader, heap manager, and other Windows components. The `PEB` is made accessible in user space because making these facilities accessible only through system calls would be prohibitively expensive.

The Windows subsystem process, _csrss.exe_, maintains a `CSR_PROCESS` data structure for every user process that it manages.

### Process Structures in Kernel Space

Every process on the system is represented internally by an executive process (`EPROCESS`) structure maintained by the executive. The majority of the `EPROCESS` structure and its related data structures exist only in system address space.

The first member of the `EPROCESS` structure is the kernel process (`KPROCESS`) structure. This structure is utilized by the lower-level components of the Windows kernel (e.g. the dispatcher) and provides a layer of abstraction between the two components (executive and kernel).

The kernel component of the Windows subsystem, _win32k.sys_, maintains a `W32PROCESS` data structure for every process that utilizes its services; this structure is initialized upon the first call made by the process into _win32k.sys_.

### Process Permissions

Process permissions are implemented by a collection of related but distinct mechanisms:

- Integrity Level
- Account Rights and Privileges: for operations performed by the process that do not involve some action taken upon a a particular object
- Access Token User and Group SIDs: for operations involving a specific object or objects 

**Integrity Level**

One of the fields maintained in the process primary access token is the integrity level for the process. This integrity level is used to perform mandatory integrity level checks during all object access operations. The integrity level denotes the "trustworthiness" of the process to which the integrity level is assigned. Windows defines the following integrity levels:
- (0) Untrusted
- (1) Low
- (2) Medium
- (3) High
- (4) System
- (5) Protected

All objects managed by the Object Manager also have an associated integrity level maintained in the object security descriptor. If any thread within a process (assuming the thread is not impersonating) attempts to access an object with an integrity level higher than that reflected by the process primary access token, the access is denied by the Security Reference Monitor.

The integrity level for a process is implemented as a SID within the process primary access token's list of group SIDs. In addition to this integrity level SID, the primary access token also contains the owner SID and any other group SIDs that compose the process security context. These SIDs are derived from the user account that was utilized to create the process. The owner SID and the group SIDs are then utilized in discretionary access control checks whenever the process attempts to manipulate an object managed by the Object Manager. This discretionary access control check (performed by the Security Reference Monitor) determines if the action requested by the process is permitted.

**Account Rights and Privileges**

A _privilege_ is the right of an account to perform a particular system-related operation, such as debugging processes or shutting down the system. 

An _account right_ grants or denies the associated account the ability to perform a particular type of logon, such as local logon or interactive logon.

Unlike privileges, account rights are NOT stored in the access token structure. Instead, account right checks are performed by the `LsaLogonUser()` function during the logon process by checking the type of logon requested against the account rights stored in the LSA policy database.

Privileges are not enforced by the SRM. Instead, privileges are enforced by various components of the system as appropriate. For instance, the `SeDebugPrivilege` privilege is checked during process creation after a call to `CreateProcess()` or `OpenProcess()` where the relevant debug access request flag is specified.

Windows distinguishes standard privileges from "super privileges" which are essentially just privileges that are identified as powerful enough to effectively make any user with this privilege present in their token a "super user."

**Access Token User and Group SIDs**

Every process has an associated access token that encapsulates the security context of that process. The access token contains information such as the process _User SID_, the _Group SIDs_ associated with the process, and the _privileges_ associated with the user account that created the process. 

### User Account Control (UAC)

User account control (UAC) is the mechanism that Windows uses to enforce basic security segmentation, although they maintain that it is not actually a "security boundary." UAC implies that users will typically run under an unprivileged (non-administrator) token, but can selectively choose to "elevate" when the rights associated with the administrator token are required to perform some operation.

Windows implements UAC by creating two tokens for administrator users during the logon process. When an administrator logs on, Windows creates the standard administrator token that encapsulates the administrator's credentials and also creates a "filtered admin" token that has many of the rights of the administrator token stripped out. Microsoft refers to this feature as "Admin Approval Mode."

As mentioned above, the process of granting administrator rights to a process is termed _elevation_. Windows recognizes two forms of elevation:

- _Over-the-Shoulder Elevation_: When a standard user needs to elevate privileges; administrator credentials must be entered to perform the operation; termed such because it is often completed by an administrator standing over the shoulder of a regular user, entering their credentials
- _Consent Elevation_: When an administrator, running under a filtered token, gives consent to the new processes to execute with administrative rights.

The dialog boxes presented by UAC prompts reveal the nature of the image being executed:

- Signed images will appear with a blue UAC prompt
- Unsigned images will appear with a yellow UAC prompt

Application developers may control the requested elevation level under which their image executes by specifying certain values in the application's manifest file.

**UAC Registry Keys**

UAC behavior is controlled by the following four (4) registry keys under `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System`:

- `ConsentPromptBehaviorAdmin`
- `ConsentPromptBehaviorUser`
- `EnableLUA`
- `PromptOnSecureDesktop`

### Process Attributes in User Space

User-space process attributes refer to those properties of Windows processes that may be manipulated from a user-mode execution context. Examples of such attributes include the following:
- Running State: the Win32 API exposes the `TerminateProcess()` function which allows one process to forcibly terminate another assuming that it is able to acquire a sufficiently-powerful handle to the target process; one may utilize this function as a method for forcibly closing unresponsive processes or as a last resort mechanism to control the execution of a child process; it is generally best to avoid this function, however, as it does not allow components within the process (e.g. DLL unload routines, C++ destructors) to execute and perform proper cleanup
- Priority Class: the `SetPriorityClass()` API allows one to set the priority class for a specified process identified by an open handle; the process priority class determines the base priority of threads executing within that process context; one may manipulate this property as a means of augmenting or curtailing the amount of processor time that the threads within the processor are allocated because thread priority is the primary determinant utilized by the kernel dispatcher's scheduling algorithm
- Affinity Mask: the `SetProcessAffinityMask()` function may be utilized to control the processor affinity for the threads in a specified process; one may utilize this function as a means of improving system performance in certain situations by explicitly specifying that specified threads may only be permitted to execute on a limited set of processors
- Mitigation Policy: the `SetProcessMitigationPolicy()` function may be utilized to set the mitigation policies for the process in whose context the calling thread executes; process mitigation mechanisms that may be manipulated via this API include data execution prevention (DEP) usage, address-space layout randomization (ASLR) usage, and control-flow guard (CFG) usage; one may utilize this API to upgrade or downgrade the security posture of the current process
- Working Set Size: the `SetProcessWorkingSetSize[Ex]()` function may be utilized to set a minimum and maximum working set size for a specified process; this property may be utilized to expand or contract the memory usage capabilities of a process
- AppContainer Sandbox: one may utilize the process creation API in conjunction with the security API to create a new process that runs in the context of an AppContainer sandbox; one may utilize this property to limit the capabilities of a child process that may need to perform some "risky" operation (such as hosting a web browser)
- Parent Process: one may utilize the process creation API to manually specify the parent process for a newly-created process; re-parenting is a procedure that is commonly utilized by the operating system in order to present more a comprehensible process hierarchy
- Virtual Address Space Content: the virtual memory API allows user-space programs with a sufficient level of privilege to both read and write the address space of other processes; this is an extremely powerful mechanism that supports high-level techniques such as DLL injection and remote process injection; the virtual APIs that support this functionality include `VirtualAllocEx()`, `ReadProcessMemory()`, and `WriteProcessMemory()`

### Process Attributes in Kernel Space

Kernel-space process attributes refer to those properties of Windows processes that may be manipulated from a kernel-mode execution context. Examples of such attributes include the following:

### Process Management API

**Process Creation**

All of the process creation functions below end in the `CreateProcessInternl()` function, which then calls `NtCreateUserProcess()` to transition to the kernel.

The following functions are available in _kernel32.dll_.

- `CreateProcess()`
- `CreateProcessAsUser()`

The following functions are availbe in _advapi32.dll_; they then make an RPC call to _seclogon.dll_ (hosted by _svchost.exe_) to continue the process creation procedure.

- `CreateProcessWithTokenW()`
- `CreateProcessWithLogonW()`

**Process Enumeration**

The simplest way of enumerating processes on the system is using the `EnumProcesses()` function available in `<psapi.h>`. However, this API only exposes a very limited amount of information regarding all of the processes on the system.

A more robust mechanism for enumerating processes is via the "toolhelp" functions, available in the `<tlhelp32.h>` header:

- `CreateToolhelp32Snapshot()`
- `Process32First()`
- `Process32Next()`

The Windows Terminal Services (WTS) API also provides several functions that perform process enumeration. These functions are available in the `<wtsapi32.h>` header and the `wtsapi32.lib` library must be linked against in order to use them.

- `WTSEnumerateProcesses()`
- `WTSEnumerateProcessesEx()`

A final option for enumerating processes is using the native API directly. The `NtQuerySystemInformation()` native API supports the querying of various system information classes, including process information. Of course, in order to use this function, one must know the structure of the `SYSTEM_PROCESS_INFORMATION` type in which it store its results.

Note: the [Process Hacker](https://github.com/processhacker) project maintains a collection of header files that contain function signatures and structure definitions for many of the native APIs.

### Process Kernel Debugger Commands

```
// process swiss-army knife
!process

// dump the EPROCESS structure
dt nt!_EPROCESS

// dump the KPROCESS structure
dt nt!_KPROCESS
dt nt!_EPROCESS Pcb.

// switch process context
.process /P <EPROCESS PTR>

// dump the PEB
!peb <PEB ADDR>

// dump the CSR_PROCESS
dt csrss!_CSR_PROCESS
```

### References

- _Windows System Programming, 4th Edition_ Pages 181-220
- _Windows Internals, 7th Edition Part 1_ Pages 101-154
- _Windows Internals, 7th Edition Part 1_ Pages 722-734
- _Windows 10 System Programming_ Chapter 3
