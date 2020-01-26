## Windows Drivers and Driver Development

### Build Environments

**Driver Development Kit (DDK)**

The DDK is the now-deprecated name for the Windows driver development kit. The name was changed to the WDK with the release of Windows Vista.

**Windows Driver Kit (WDK)**

WDK is the new name for the Windows driver development kit (changed in Vista).

The WDK is a software toolset that enables device driver development. It includes documentation, samples, build environments, and developer tools. However, it does not include a compiler implementation, Visual Studio build tools, or the Windows SDK, all of which are required to develop Windows drivers.

**Enterprise Windows Driver Kit (EWDK)**

The EWDK is a standalone, self-contained command line environment for building drivers. It includes the Visual Studio build tools, the SDK, and the WDK. That is, in contrast to the WDK, it does not require separate installation and configuration of Visual Studio to be a fully-functional driver development environment.

### Kernel Driver Execution Contexts

Kernel mode drivers execute in one of the three following contexts:

- In the context of the thread that initiated some IO operation
- In the context of some system thread
- In the context of some arbitrary thread as a result of running in response to an interrupt

### Driver Types

Windows generally recognizes the following "types" of device drivers:

- Hardware device drivers
- File system drivers
- File system filter drivers
- Network redirectors and servers
- Protocol drivers
- Kernel streaming filter drivers
- Software drivers

### The Windows Layered IO Concept

The Windows IO subsystem uses a layered driver / device model. Under this model, a request is initially presented to a given driver, and from there the driver may either handle the request itself or may pass the request on to another driver for it to handle. In this way, an IO request is passed from driver to driver along a "branch" of the overall device tree until the request is completed. Each driver that ultimately processes an IO request determines where to send the request next (or whether to forward the request at all). Furthermore, any driver can send any IO request to any device object. Thus, the path that an IO request will take within the system to reach completion is never predetermined. 

### NT4 Drivers

The original driver model was introduced in the first NT version (3.1). 

### Windows Driver Model (WDM) Drivers

Originally, WDM was an extension to the NT4 driver model introduced with Windows 2000. 

In WDM, there are three (3) kinds of drivers:

- Bus drivers -> a bus driver services any device that has child devices; they are generally provided by MS
- Function drivers -> the driver that directly handles a specific device; these drivers are those that know the most about the particular device they service, they are generally the only ones that interact with device-specific registers
- Filter drivers -> filter drivers augment the functionality of existing devices or drivers

The major implication of the above taxonomy: in WDM, no single driver is completely responsible for a device. Instead, a device is managed through the cooperation and interoperation of bus, function, and possibly filter driver(s). 

### Windows Driver Foundation (WDF) Drivers

The WDF was introduced with the intention of simplifying driver development. It introduces two new driver development frameworks:

- Kernel Mode Driver Framework (KMDF)
- User Mode Driver Framework (UMDF)

KMDF adds a layer of abstraction over WDM that greatly simplifies driver development without altering the underlying architecture of the WDM.

UMDF allows for the development of drivers that run in user mode, typically as services. Requests from UMDF drivers are serviced by a wrapper driver that runs in kernel mode. This setup has the obvious benefit of removing many of the horrible consequences of a driver vulnerability or bug.

Note that UMDF 1.x presented a COM-based interface while KMDF presents an object-oriented C interface. UMDF 2.x was updated to be consistent with the KMDF interface, meaning that drivers written using either framework may be easily ported to the other.

**KMDF Drivers**

The KMDF library is implemented in a kernel driver with an image name similar to _Wdf001000.sys_. The image name for the KMDF library driver may change as updated versions of KMDF are released. However, all versions of the KMDF library driver will remain present on the system, allowing KMDF drivers compiled against earlier versions of the framework to continue to function. 

The structure of a KMDF driver is similar to the structure of a WDM driver:

- Initialization routine: a `DriverEntry()` function that initialized the driver and registers it with the framework; for software drivers this is where the device object is created
- An add-device routine: for PnP drivers
- One or more `EvtIo*` queues: similar to the dispatch functions in a WDM driver

KMDF drivers may be simpler to implement than their WDM brethren because the KMDF handles many of the common operations on behalf of the driver, and the driver only has to handle events for which the KMDF does not define a default. Optionally, the KMDF driver may override the KMDF default in order to perform custom processing in response to some event.

As previously mentioned, KMDF implements an object model via object oriented C. The object model implemented by KMDF is distinct from the object model utilized by the Windows executive / kernel, meaning that the object types and the namespace are completely separate. Additionally, whereas the object model for executive objects is "flat," the KMDF object model is inherently hierarchical. All objects in the KMDF object model are children of the `WDFDRIVER` object that represents the driver itself.

Some important KMDF object types include:

- `WDFDRIVER`: the root object of the KMDF object hierarchy
- `WDFDEVICE`: represents an instance of a device; a direct child of the `WDFDRIVER` object
- `WDFREQUEST`: represents an IO request
- `WDFQUEUE`: a generic IO queue that is the child of a device; common queue children include:
    - `WDFDPC`
    - `WDFTIMER`
    - `WDFWORKITEM`

Another important component of KMDF objects is their support for _context_. This is similar to the idea of the device extension structure of a device object in WDM, but has been generalized in KMDF to allow any KMDF object to maintain arbitrary context, allowing different sections of code to access different user-defined data associated with the object based on the current context.

KMDF IO processing is based on queues. Under the hood, the KMDF library driver manages IRPs delivered to a device by the IO Manager, and after performing some initial bookkeeping, packages up the IO request into a `WDFREQUEST` object and forwards it to an appropriate queue maintained by the KMDF driver itself. 

**UMDF Drivers**

The object and IO models for KMDF and UMDF 2.x are identical. However, there remain some difference between the two frameworks because of the inherent differences between user mode and kernel mode execution. Some of these differences include:

- Crashing a UMDF driver does not crash the system
- UMDF host process runs in the context of the Local Service account, so its privileges are relatively limited by default
- IRQL is always 0 (`PASSIVE_LEVEL`) so the driver need not be concerned with the distinction between pageable and nonpageable memory, etc.
- A UMDF driver may be debugged as a user mode application; no kernel debugging setup is required

UMDF drivers run within a driver host process, _WUDFHost.exe_. This host process is analogous to a Windows service hosting process (e.g. _svchost.exe_); note, however, that such host processes are not true Windows services as they are not managed by the SCM (instead they are managed by the driver manager, described below). The host process contains the driver image itself, the UMDF library (implemented as a DLL, _WUDFx02000.dll_), and a runtime environment (also implemented as a DLL, _WUDFPlatform.dll_) that handles things like IO dispatching, communication with the kernel, and the thread pool.

An important component of UMDF is the _reflector_. This is a standard WDM driver that sits at the top of the device stack for devices that are managed by UMDF drivers. The reflector acts as the intermediary between IO requests sent from user mode applications ("clients") to the UMDF driver host process by accepting IO requests from the kernel IO manager and forwarding them along appropriately. 

The reflector for a UMDF driver does not communicate directly with the UMDF driver itself, but rather communicates with the Driver Manager - a standard Windows service running in user mode. The driver manager is implemented in _WUDFsvc.dll_ and is hosted by an instance of _svchost.exe_. The driver manager receives requests from the reflector running in the kernel and relays them to the appropriate UMDF driver. In addition, the driver manager service is also responsible for starting and stopping UMDF host processes.

### Universal Windows Drivers

Universal Windows Drivers refers to the ability to write drivers that implement a common API and may be deployed on a variety of platforms e.g. IOT, phones, Xbox, etc. Universal Windows Drivers may use any of the frameworks mentioned above as their driver development model.

### References

- _Windows Internals, 7th Edition Part 1_ Pages 578-590

