## Windows System Architecture: Drivers and Driver Development

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

### High-Level Overview: The "Layered" Approach

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

### Universal Windows Drivers

Universal Windows Drivers refers to the ability to write drivers that implement a common API and may be deployed on a variety of platforms e.g. IOT, phones, Xbox, etc. Universal Windows Drivers may use any of the frameworks mentioned above as their driver development model.

