## Windows AppContainers and Process Isolation

_AppContainers_ are a security sandbox feature introduced in Windows 8.

### Overview

AppContainers were originally designed to host Universal Windows Platform (UWP) applications, but they can be used to host any program. UWP applications are simply those that host the Windows Runtime.

The original codename for AppContainers was _LowBox_. 

### UWP vs Traditional Desktop Applications

A high-level comparison of UWP and "traditional" Windows desktop applications is presented in the table below.

|                | UWP Application                         | Classic Application         |
|----------------|-----------------------------------------|-----------------------------|
| Device Support | All Windows device families             | Desktop only                |
| APIs           | WinRT, subset of COM, subset of Win32   | COM, Win32, subset of WinRT |
| Identity       | Strong app identity                     | Raw EXEs and processes      |
| Information    | Declarative APPX manifest               | Opaque binaries             |
| Installation   | Self-contained APPX package             | Loose files of MSI          |
| App Data       | Isolated per-user / per-app storage     | Shared user profile         |
| Lifecycle      | Participates in app resource management | Process-level lifecycle     |
| Instancing     | Single instance only                    | Any number of instances     |

The big takeaway from the architecture of UWP apps versus classic desktop applications is that where a classic desktop application relies directly on the Win32 API (e.g. calls into it directly), UWP applications reside at a higher level of abstraction and rely directly upon the Universal Windows Platform (i.e. Window Runtime APIs) in order to operate.

The Windows Runtime APIs are based on an enhanced version of COM. 

### UWP and AppContainer Process Characteristics

- Process token integrity level = AppContainer (low)
- UWP applications always run in the context of a job, one job per app
- Token possesses an AppContainer SID, which, for UWP apps, is based on SHA-2 hash of app package name
- The AppContainer SID is derived from the `APPLICATION PACKAGE AUTHORITY` rather than `NT AUTHORITY`
- Token may declare capabilities which are also identified by a SID derived from `APPLICATION PACKAGE AUTHORITY`
- Token is limited to possessing the following privileges:
    - `SeChangeNotifyPrivilege`
    - `SeIncreaseWorkingSetPrivilege`
    - `SeShutdownPrivilege`
    - `SeTimeZonePrivilege`
    - `SeUndockPrivilege`
- The `TOKEN_LOWBOX` flag will be set in the token's `Flags` member, identifying this token as belonging to an AppContainer

### Viewing an AppContainer's Token and Capabilities

Use the `accesschk` tool from SysInternals to view an application's token along with its capabilities.

```
accesschk.exe -p -f <PID>
```

The _Security_ tab of Process Explorer will also display an AppContainers capabilities in SID form.

### Brokers

Some operations cannot be performed directly by an AppContainer and require the help of another process. Windows provides such helper processes in the form of _brokers_.

Brokers are dynamically created to service a request from an AppContainer at the time the request is generated. The AppContainer communicates with the system broker manager, _RuntimeBroker.exe_, via an ALPC channel and the system broker manager handles the creation of the new broker process to service the AppContainer's request.

### References

- _Windows Internals, 7th Edition Part 1_ Pages 684-709