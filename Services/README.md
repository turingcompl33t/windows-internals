## Windows Services

Windows services add functionality to traditional desktop applications; namely, they allow programs to be started, stopped, paused, resumed, and monitored, all via a common API.

Windows services consist of three (3) components:

- A Service Application
- A Service Control Program (SCP)
- The Service Control Manager (SCM)

A service application implements the service itself, while a service control program acts as a client application with which users may interact with a service. Windows provides several built-in SCPs, but services may also provide their own. The service control manager is the centralized management program for all Windows services.

### Service Applications

Service applications are regular Windows executables with some added required semantics:

- Receive commands from the SCM
- Send responses and status updates to the SCM

The configuration data specified in a call to `CreateService()` (used to, as the name implies, create a new service) is stored in the registry at `HKLM\SYSTEM\CurrentControlSet\Services\<SERVICE_NAME>`.

A single service process may host multiple logical Windows services. The entry point for the service process executable (e.g. `main()`) must register each logical service that it implements and subsequently invoke `StartServiceCtrlDispatcher()` to begin communications with the SCM. The SCM then sends service start commands (via a named pipe) to the service process, one for each logical service the process has registered. For each service start command this "main service process thread" receives,it starts a new thread to begin execution at the logical service entry point. The original invocation of `StartServiceCtrlDispatcher()` blocks indefinitely, waiting on commands from the SCM, while the logical services execute.

The logical service must register a function to handle communications from the SCM via the `RegisterServiceCtrlHandler()` function. The main service process thread (waiting in `StartServiceCtrlDispatcher()`) then invokes the registered function whenever it receives a command from the SCM destined for this specific logical service.

**Service Accounts**

Service applications typically execute under one of three (3) accounts. These accounts have varying levels of permissions based on the groups of which they are a member.

Local System: this is the default account under which service applications run.

- Everyone
- Authenticated Users
- Administrators

Network Service: this is the account used by services that need to perform network communications but do not require administrator privileges.

- Everyone
- Authenticated Users
- Users
- Local
- Network Service
- Service

Local Service: this account is just like the Network Service account except it may only access network resources that allow anonymous (unauthenticated) access.

- Everyone
- Authenticated Users
- Users
- Local
- Local Service
- Service

### The Service Control Manager (SCM)

The executable file that implements the SCM is _Services.exe_. The Windows initialization process (_wininit_) starts the SCM during the system boot sequence.

The entry point of the SCM is `SvcCtrlMain()`. This function then calls `ScGenerateServiceDB()` to initialize the SCM's internal service database. It also calls `ScGetBootAndSystemDriverState()` to query the results of boot and system driver initialization (load and start) performed by the IO Manager earlier in the boot sequence.

Finally, the SCM creates an RPC named pipe (`\Pipe\Ntsvc`) to listen for requests from SCPs and signals its startup synchronization event to declare that it is ready to accept `OpenSCManager()` requests from the SCPs.

This named pipe that the SCM uses to communicate with SCPs is distinct from the named pipe that it uses to communicate with service processes. When the SCM starts a service, it creates a named pipe `\Pipe\Net\NtControlPipeX` (where `X` is a number that increments monotonically) which the service processes then connects to in `StartServiceCtrlDispatcher()`. 

The SCM then proceeds to start all services registered as auto-start services, including those marked as "delayed auto-start."

**Boot and System Initialization Failures**

A successful system boot consists of:

- A successful user logon
- Successful startup of auto-start services, including device drivers

Boot fails in the event that a device driver crashes the system during boot, or an auto-start service fails to start with an error value above a certain threshold. Recall that the SCM is not directly responsible for starting boot and system start device drivers, but it is responsible for querying the results of driver loads and starts performed earlier by the IO Manager.

Because the SCM is in a position to "know" the startup status of all auto-start drivers and services, it is charged with determining when the system's current registry configuration (`HKLM\SYSTEM\CurrentControlSet`) should be saved as the "last known good" configuration which is also stored in the registry.

**Shared Service Processes**

It is common to see a single process host multiple logical services. For instance, the `LSASS` process actually hosts multiple security-related services, and `Svchost` is a generic multi-service host process, of which there are typically many instances running.

One simple way to view the services hosted by each process on a system is via the _tasklist.exe_ utility, included with Windows.

```
tasklist.exe /SVC
```

### Writing Service Applications

Programs that are designed to run as Windows services resemble device drivers in the sense that they register a set of callback functions in order to respond to runtime requests (start, stop, etc.). However, whereas device drivers (implicitly) register with the IO Manager, service applications register with the Service Control Manager (SCM). 

The entry point of a service application (`main()`) simply registers all of the logical services implemented by a particular program (a program may implement multiple logical services, but it is common practice to only implement a single logical service in a program) with the SCM via the `StartServiceCtrlDispatcher()` function. This registration specifies the entry point of each logical service.

The logical service entry point must first register the service control handler for the service. This effectively registers the callback function that will be invoked when the SCM sends control signals to the service. Registering this handler is accomplished via the `RegisterServiceCtrlHandler()` function. The second thing the logical service entry point must do is update the service status to `SERVICE_START_PENDING` via the `SetServiceStatus()` API. Once this has been accomplished, the service may perform its own internal initialization and begin service-specific work.

### Interacting with the Service Control Manager

The commandline tool `sc.exe` allows one to interact with the SCM. However, it is also possible to interact with the SCM programmatically via the following APIs.

Open the SCM with `OpenSCManager()`.

Create a new service with `CreateService()`. This function takes 13 parameters - perhaps the most in the entire Win32 API. There is a corresponding `OpenService()` function to open an existing service by name. Close a service with `CloseService()`.

Once a service has been created or a handle to an existing service has been obtained, start the service with `StartService()`. Once the service is started, send arbitrary control signals to the service with `ControlService()`.

Finally, query the current status of a service with `QueryServiceStatus()`.

### References

- _Windows System Programming, 4th Edition_ Pages 353-379
- _Windows Internals, 6th Edition Part 1_ Pages 305-335