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

### Basic Process Management

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

### Process Lifecycle: Process Termination

A process exits gracefully by calling `ExitProcess()`. Most of the time, this function is called implicitly by the process startup code when the process's initial thread returns from the main function. Here, graceful exit entails the ability of DLLs loaded in the address space of the process to perform cleanup operations (via `DLL_PROCESS_DETACH` handlers in `DllMain()`) and that proper respect is paid to SEH termination handlers.

In contrast, a call to `TerminateProcess()` does not guarantee such a graceful exit.

### Process Attributes in User Space

The process environment block (`PEB`) is the primary process-related data structure that exists in user space. The `PEB` contains information needed by the image loader, heap manager, and other Windows components. The `PEB` is made accessible in user space because making these facilities accessible only through system calls would be prohibitively expensive.

The Windows subsystem process, _csrss.exe_, maintains a `CSR_PROCESS` data structure for every user process that it manages.

### Process Attributes in Kernel Space

Every process on the system is represented internally by an executive process (`EPROCESS`) structure maintained by the executive. The majority of the `EPROCESS` structure and its related data structures exist only in system address space.

The first member of the `EPROCESS` structure is the kernel process (`KPROCESS`) structure. This structure is utilized by the lower-level components of the Windows kernel (e.g. the dispatcher) and provides a layer of abstraction between the two components (executive and kernel).

The kernel component of the Windows subsystem, _win32k.sys_, maintains a `W32PROCESS` data structure for every process that utilizes its services; this structure is initialized upon the first call made by the process into _win32k.sys_.

### Process Permissions

Process permissions are implemented by a collection of related but distinct mechanisms:

- Account Rights and Privileges: for operations performed by the process that do not involve some action taken upon a a particular object
- Access Token User and Group SIDs: for operations involving a specific object or objects 

**Account Rights and Privileges**

A _privilege_ is the right of an account to perform a particular system-related operation, such as debugging processes or shutting down the system. 

An _account right_ grants or denies the associated account the ability to perform a particular type of logon, such as local logon or interactive logon.

Unlike privileges, account rights are NOT stored in the access token structure. Instead, account right checks are performed by the `LsaLogonUser()` function during the logon process by checking the type of logon requested against the account rights stored in the LSA policy database.

Privileges are not enforced by the SRM. Instead, privileges are enforced by various components of the system as appropriate. For instance, the `SeDebugPrivilege` privilege is checked during process creation after a call to `CreateProcess()` or `OpenProcess()` where the relevant debug access request flag is specified.

Windows distinguishes standard privileges from "super privileges" which are essentially just privileges that are identified as powerful enough to effectively make any user with this privilege present in their token a "super user."

**Access Token User and Group SIDs**

Every process has an associated access token that encapsulates the security context of that process. The access token contains information such as the process _User SID_, the _Group SIDs_ associated with the process, and the _privileges_ associated with the user account that created the process. 

### User Account Control (UAC)

User account control (UAC) is the mechanism that Windows uses to enforce basic security segmentation, alhtough they maintain that it is not actually a "security boundary." UAC implies that users will typically run under an unprivileged (non-administrator) token, but can selectively choose to "elevate" when the rights associated with the administrator token are required to perform some operation.

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

### Process Management API

**Process Creation**

All of the process creation functions below end in the `CreateProcessInternl()` function, which then calls `NtCreateUserProcess()` to transition to the kernel.

The following functions are available in _kernel32.dll_.

- `CreateProcess()`
- `CreateProcessAsUser()`

The following functions are availbe in _advapi32.dll_; they then make an RPC call to _seclogon.dll_ (hosted by _svchost.exe_) to continue the process creation procedure.

- `CreateProcessWithTokenW()`
- `CreateProcessWithLogonW()`

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
