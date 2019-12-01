## Windows Services

Windows services add functionality to traditional desktop applications; namely, they allow programs to be started, stopped, paused, resumed, and monitored, all via a common API.

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