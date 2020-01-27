## Windows IO System

The Windows IO Manager provides the system infrastructure for all IO operations:
- Device instances are represented by a _device object_
- Driver (modules that service devices) instances are represented by a _driver object_
- Open instance of a device is represented by a _file object_

All IO operations are then performed on file objects, rather than device objects themselves - the file object acts as a handle to the underlying device object. Common IO methods include:
- Create
- Close
- Read
- Write 
- DeviceIoControl

The IO manager manages hardware driver loading and unloading. The IO manager loads drivers dynamically when a supported device is first discovered. By default, the code and data utilized by a kernel driver is non-pageable (although this property is under the control of the driver developer). One driver instance may support multiple device instances. When a driver is first loaded, the IO system invokes the driver at its `DriverEntry()` entry point. 

The IO manager is also responsible for loading and unloading software drivers. Software drivers are those that do not manage a physical hardware device, and are also sometimes known as _pseudo-device drivers_ or _kernel services_. 

There are two general types of software drivers:
- Plug-and-Play aware ("root enumerated")
    - Work and structured exactly like a PnP aware hardware driver
    - Enumerated by the PnP Manager which creates a fake device arrival to trigger initialization of the driver
- Legacy 
    - By far the more common of the two types of software drivers
    - Pre-PnP, NT V4 style drivers
    - Used for the majority of kernel services, file systems, etc.
    - May be loaded manually (via the SCM) or via pre-set Registry entries (boot-start, system-start, etc.)

### IO Processing

IO processing is a fundamental system mechanism implemented by the Windows IO Manager. The IO processing procedure involves all layers of the system - from user-mode application code to low-level device drivers that manage physical hardware. The procedure below gives a general overview of the IO processing procedure. This example uses the `ReadFile()` operation as a case study. Note that propagation of the IRP down the device tree is not discussed even though it is likely that most IO operations originating with the `ReadFile()` call will involve a multi-layer device stack and perhaps even lateral movement within the device tree.

1. Application code running in user mode calls `ReadFile()` which eventually transitions to _ntdll.dll_ implementation of `NtReadFile()`; this function performs the transition to kernel mode
2. The IO Manager implements the executive function `NtReadFile()` in the kernel, some validation checks are performed, then the IRP for the request is initialized and directed at the appropriate device object that represents the target for the IO operation; the pointer to the driver object maintained by the device object allows the IO Manager to locate the appropriate driver dispatch routine for the operation
3. The driver dispatch routine gives the driver code its first look at the IRP; the driver then wants to invoke the appropriate Start IO routine to service the request, but if the hardware device is busy the IRP is inserted into a queue managed by the device object and a busy bit is set for the device object reflecting the fact that is has requests pending, the `IoStartPacket()` function is
typically used to handle such scenarios (this example assumes the driver is utilizing system queueing rather than driver queueing)
4. When the hardware device is ready, the Start IO routine is called (either from the dispatch routine directly or from the IO manager if `IoStartPacket()` was used to handle the request when the hardware device was busy) to initiate the IO with the hardware device; the purpose of the Start IO routine is to perform the actual programming of the hardware device to perform the requested IO operation once the Start IO routine returns; the hardware is left to do its thing and the device can now service other requests
5. When the hardware completes the requested operation it raises an interrupt; the hardware handles the transition from the interrupt to the registered ISR, raising the IRQL to the device IRQL (DIRQL) in the process
6. The ISR, running at some high Device IRQL, does as little work as possible; as its final act it queues a DPC for further processing of the completed IO operation
7. Once the interrupt is dismissed, kernel logic notices that the CPU DPC queue is not empty and issues a software interrupt at IRQL DPC_LEVEL (2) to enter the DPC processing logic
8. The DPC is dequeued in some arbitrary thread context; in the context of some kernel worker thread, the first thing it does is call the `IoStartNextPacket()` routine for the device in order to start processing of the next available IRP, this calls the Start IO routine and the cycle starting from (4) repeats for this next IRP; the DPC finally completes the IRP by calling `IoCompleteRequest()`
9. Finally, the original requesting thread needs to be notified that IO has completed; this is accomplished by queueing a special kernel APC for the requesting thread to be run by the requesting thread once it is given CPU time (note that for the delivery of such APCs, the originating thread need not enter an alertable wait state as is the case for standard user APCs)

This final step is slightly different in the case of asynchronous IO requests. In the asynchronous case utilizing extended IO, the application code supplies a callback function and the IO Manager queues a user APC in the requesting thread's APC queue as the last stage in IRP completion. In this case the requesting thread must enter an alertable wait state in order for the APC queue to be checked and the process to be notified of the completion of the IO operation.

### IO Manager Data Structures

The IO Manager utilizes several critical data structures to implement IO processing.

A driver object is created by the IO Manager when a driver is loaded. A pointer to this object is then passed to the driver itself at its `DriverEntry` entry point. The driver object contains information related to the driver itself, such as where the driver is loaded, how big it is, what device objects it owns, etc. In its initialization routine, the driver fills in fields of its own driver object with important information, including:
- Pointer to unload routine
- Pointers to dispatch routines within its major function array
- Pointer to driver extension, if present

A device object is the external interface to a device driver. A device object may represent either actual hardware (e.g. disk, CD, COM port, mouse, etc.) or a driver-created abstraction such as a volume, file system, or channel. Device objects contain device attributes that characterize the device such as the device type, the buffering method read, write, and device IO control operations, etc. Like driver objects, device objects also have the ability to allocate a memory region for device-specific context called its device extension. API calls like `CreateFile()` are ultimately always directed at a device object. IO requests that target a device object are then handled by the driver that created that device object.

A file object is the Windows object on which IO is performed. The file object represents a single open instance of a device object, and applications invoke functions on a file object in order to generate requests to the underlying device object.

### IO Request Packets (IRP)

Every IO request is described to the IO subsystem by an IO Request Packet (IRP). This data structure completely describes the IO request - no other information or context is needed to process the request. Important attributes of the request described by the IRP include:
- The operation to perform (e.g. read, write, device IO control)
- The description of one or more data buffers utilized in the request
- The file object at which the request is directed
- The execution mode of the requester (user-mode or kernel-mode)
- The requesting thread context

On receiving an IO request, the IO Manager constructs an IRP based on information that is present in the device object at which the request is ultimately targeted (the device object that underlies the file object on which the IO was initially invoked). Once this is complete, the IO Manager then invokes the appropriate function defined by the driver that owns the device object with a pointer to the IRP for the request so that the driver may process it.

IRPs are a critical component of the overall Windows IO model - encapsulating each IO request in an "atomic" IO request packet is what allows for the flexibility of the layered approach, and furthermore is a design component that contributes to the fundamentally asynchronous nature of the Windows IO subsystem.

### Filter Drivers

Filter drivers are those drivers that modify or add functions to an existing device. A filter driver defines a filter device object - an unnamed device object. The filter driver then attaches this filter device to an existing device stack to intercept or otherwise process requests that target the devices in the stack.

Filter devices are defined in terms of the device stack's function device object (FDO):
- Upper Filters: attached above the FDO within the device stack; upper filters process IO requests before the driver that owns the FDO
- Lower Filters: attached below the FDO within the device stack; lower filters process IO requests after the driver that owns the FDO

There may be multiple filters in a single device stack. The ordering of the filters is non-deterministic - the only guarantee is the placement of the filter in the stack relative to the FDO. 

The operation of filter drivers is relatively straightforward once the filter device has been attached to the device stack. On receiving an IO request, the IO Manager locates the initially targeted device object, and from there it walks the list of device (via the `AttachedDevice` pointer of the device object) "upwards" until it reaches the uppermost device. This device object at the top of the device stack is then the one that actually receives the IO request.

Generic filter drivers that utilize the approach described here are essentially obsolete. Filesystem drivers that wish to perform IO filtering should now utilize the fileystem minifilter driver model to perform this task, while network filtering should be performed via the functionality exposed by the Windows Filtering Platform (WFP).

### IO Request Queues

In certain circumstances a driver may need to queue IRPs rather than processing them immediately. The reason for doing this largely depends on the type of driver in question:
- Hardware drivers: hardware drivers may implement queueing to account for situations in which IO request arrive at the device they manage faster than they can be processed
- Software drivers: software drivers may want to queue an IO request and complete it later in response to an asynchronous event; this method may be used to wait for further data to be gathered, or as a way of implementing an "inverted call model" in which the driver is able to notify a user-mode client of some event by completing the request

There are two options available for implementing queueing:
- The driver may rely on the IO Manager to implement queueing on its behalf (_system queueing_)
- The driver may implement its own queueing mechanism (_driver queueing)_

Under the system queueing implementation, a driver simply calls the `IoStartPacket()` function to notify the IO Manager that the IRP in question should be queued for later processing. Once the system reaches a point in which there a no further requests pending on the device, the IO Manager begins processing the queued IRPs by calling the driver's `StartIo()` routine. 

Under the driver queueing implementation, the driver itself explicitly implements queueing. The driver may perform this queueing by, for instance, maintaining a list (linked list via `LIST_ENTRY` structures) of queued IRPs. This approach is more flexible than the system queueing implementation, but involves significantly more work for the driver developer.

### Serialization

In the context of Windows kernel development, serialization refers to the need to address the inherently reentrant environment in which kernel drivers operate:
- The same driver code may be executing on multiple processors simultaneously
- The same driver code may be re-entered by multiple threads on a single processor due to scheduling

Because of the need to support reentrancy, Windows kernel drivers must utilize explicit serialization techniques to ensure that parallel and concurrent operations do not corrupt data managed by the driver. To this end, Windows provides two general categories of locks to implement serialization:
- Wait Locks: wait locks implement scheduler-based waiting - they render a thread non-dispatchable until some condition is satisfied; wait locks may only be utilized when a thread is running dispatchable
- Spin Locks: spin locks implement busy waiting - they cause the waiting thread to spin idly until some condition is satisfied; spin locks may only be utilized when a thread is running non-dispatchable

Examples of dispatcher objects that support scheduler-based waiting include:
- `KEVENT`: used for simple event signaling
- `KSEMAPHORE`: a generalization of an event with an internal count
- `KMUTEX`: a generic mutual exclusion object that supports exclusive ownership

On top of these simple kernel dispatcher object, the Windows executive provides several synchronization objects:
- `ERESOURCE`: an implementation of the reader/writer lock model in which the resource may be acquired in a shared or exclusive mode
- `FAST_MUTEX`: a lighter-weight version of the `KMUTEX` that cannot be recursively acquired

Spin locks are utilized in cases when a thread is running non-dispatchable, that is, the current processor IRQL is greater than DISPATCH_LEVEL which implies that the kernel dispatcher cannot run and therefore that the currently executing thread will not be preempted. All spin lock instances have an IRQL implicitly associated with them. This IRQL must be the highest IRQL at which an attempt to acquire the lock will be made. For instance, the implicit IRQL for Executive spin lock objects is DISPATCH_LEVEL while the implicit IRQL for interrupt spin locks is the device IRQL associated with the device that utilizes the lock.

### Synchronous and Asynchronous IO

The Windows programming interface supports both synchronous and asynchronous IO models. These two IO models present different semantics at the level of the application developer, but are actually implemented in a nearly-identical manner at the level of the IO subsystem.

In a synchronous IO operation, the user-mode application initiates a request via the invocation of an appropriate API function (e.g. `ReadFile()`, `WriteFile()`, etc.) and this call subsequently blocks the current thread of execution until the IO operation is complete. Most functions exposed by the Win32 API that perform IO operate synchronously by default.

In an asynchronous IO operation, the user-mode application initiates a request via the invocation of an appropriate API function and this call immediately returns to the caller - the current thread of execution does not block and wait for the IO operation to complete. The user-mode application must then synchronize its retrieval of the result of this operation with the IO subsystem. There are various methods for implementing this second step. For instance, the application may utilize the overlapped IO model and wait on an event object which becomes signaled when the IO completes. Alternatively, the application may utilize the extended IO model with completion routines and specify a callback routine at the time the IO is initiated which will then be invoked when the operation completes (provided the application enters an alertable wait state).

Regardless of whether the IO operation is initiated as a synchronous or asynchronous operation at the level of the application, the IO is performed asynchronously by the IO subsystem. That is, the kernel drivers that implement the routines that ultimately service the IO operation return the result of the operation to the IO Manager as soon as the result is available. The IO Manager then provides the illusion of synchronous or asynchronous IO at the level of the application by returning the result to the application in a manner dictated by the IO method it specified when it originated the request:
- If the application initiated the operation under the synchronous model, the IO Manager returns the result to the application as soon as it becomes available
- If the application initiated the operation under the asynchronous model, the IO Manager returns the result to the application at a time and in a manner dictated by the specifics of the asynchronous IO mechanism it utilized (e.g. overlapped IO vs extended IO)

### Kernel Callbacks

Kernel callbacks are functions that are defined and registered with the system to be invoked at some point in the future in response to the occurrence of some event. The system defines a number of kernel callback mechanisms that drivers may take advantage of to receive notifications for a wide variety of system events. 

Executive callbacks are one form of kernel callback. Executive callbacks provide a generic callback notification mechanism for driver-to-driver communication. Executive callbacks are created or opened by name via the `ExCreateCallback()` function. Consumer drivers may then register a callback to be executed when the callback is notified via the `ExRegisterCallback()` function. Similarly, a producer driver notifies the callback with `ExNotifyCallback()` which results in each of the callbacks registered by other drivers being invoked serially. The producer driver may specify up to two callback-defined arguments when performing notification that are then received by the callback functions that were previously registered. Executive callbacks are designed for single-producer / multiple-consumer situations.

Microsoft currently documents two system-defined executive callbacks for which drivers may register:
- `\Callback\SetSytemTime`: notify whenever a component changes the system time
- `\Callback\PowerState`: notified by the system for a variety of power state changes

Process creation callbacks allow a driver to register for notification for two process-related events:
- Process creation
- Process exit

The legacy version of the routine used to register for process creation callbacks is `PsSetCreateProcessNotifyRoutine()`. Windows Vista SP1 introduced the `PsSetCreateProcessNotifyRoutineEx()` function which extends the legacy routine in the following ways:
- Enables support for blocking process creation within the callback body
- Provides the callback routine with additional information regarding the target of process creation via an additional argument that it is provided

Finally, Windows 10 RS2 introduced the `PsSetCreateProcessNotifyRoutineEx2()` routine which updates the previous version by invoking any registered callbacks when Linux processes running on top of WSL are created or exit.

Thread creation callbacks are similar to process creation callbacks: they allow a driver to register for notification for two thread-related events:
- Thread creation
- Thread exit

A driver registers for thread creation callbacks via either the `PsSetCreateThreadNotifyRoutine()` or the `PsSetCreateThreadNotifyRoutineEx()`. The latter function was introduced with Windows 10 and, like the updated API for process creation callbacks, provides driver writers with additional power and information within the body of the callback.

Image load callbacks allow a driver to be notified when an executable image is loaded into memory. Drivers register for image load callbacks via the `PsSetLoadImageNotifyRoutine()`. Once an image load callback has been registered, the callback is invoked whenever a user-mode executable image or kernel driver image is mapped (on driver load). Driver authors may not block image loads within the body of the callback routine. 

Registry filtering callbacks allow drivers to be notified of all registry operations that are performed in the system. The legacy API for registering for registry filtering callbacks is `CmRegisterCallback()` while newer versions of Windows include support for the `CmRegisterCallbackEx()` function. Both APIs allow the callback to block registry operations, but only the latter supports full post-processing operations.

Object manager filtering callbacks allow drivers to register to be notified when a `HANDLE` is created or duplicated. The callback routine is guaranteed to execute within the context of the thread that initiated the `HANDLE` operation. The callback routine is notified for both user-mode and kernel-mode `HANDLE` events. The API for `ObRegisterCallbacks()` is unique in that it allows for the specification of distinct pre-operation and post-operation callbacks. The pre-operation callback is called before a `HANDLE` is created or duplicated, while the post-operation callback is called after the `HANDLE` is created or duplicated. The pre-operation callback cannot fail the request, but it can modify the access rights granted to the handle. The post-operation callback cannot manipulate the request in any way.

### Hooking Techniques

Hooking techniques in the context of kernel development refer to the various methods by which control flow may be diverted from the expected path to some code under the control of the component performing the hooking. There are a variety of mechanisms for installing hooks in the Windows kernel; some of the more well-known methods are discussed below.

Perhaps the most coarse-grained method available for kernel-mode control flow redirection is via hooking the Interrupt Descriptor Table (IDT). The IDT is a system-wide array of descriptors that each contain a pointer (in an indirect, obfuscated way) to a routine that will be invoked in the event that an interrupt occurs. Thus, hooking the IDT consists of modifying the pointer contained within the table to point to a function of one's choosing that will then be invoked whenever the interrupt whose entry was overwritten occurs. As previously mentioned, this approach is very coarse-grained. For instance, the IDT maintains only a small number of entries (fewer than 256) and each entry may be utilized by a wide range of system components to operate. For this reason, the most modern kernels implement IDT protection mechanisms that will immediately bugcheck the system in the event that a modification to the IDT is detected. Furthermore, even if the system itself does not detect IDT manipulation, any rootkit detection software written after 1995 includes a check for IDT hooks. This method of kernel hooking should therefore be avoided.

Another relatively coarse-grained approach to hooking kernel-mode code is via installtation of processor model-specific register hooks. Processor model-specific registers (MSRs) are those registers that are specific to a particular process implementation. Such registers are frequently utilized by the system to perform important system operations such as maintaining system-wide debug information, maintaining memory segment information, and performing transitions between processor modes. For instance, on 32-bit systems, the `SYSENTER` instruction is utilized by the code in _ntdll.dll_ to perform a context switch to kernel-mode. The `SYSENTER` instruction reads values from three MSRs to perform this transition:
- `IA32_SYSENTER_CS`
- `IA32_SYSENTER_EIP`
- `IA32_SYSENTER_ESP`

Specifically, the instruction (eventually) transfers control to the routine at the location specified by `CS:EIP` after `CS` has been loaded from `IA32_SYSENTER_CS` and `EIP` has been loaded from `IA32_SYSENTER_EIP`. Thus, one may redirect control flow by modifying the values stored in these registers. MSR modification is performed via the `RDMSR` and `WRMSR` instructions which are privileged - they may only be executed from a kernel-mode context. Thus, one must have already achieved kernel-mode execution in order to perform this hooking technique. Assuming the kernel-mode execution is already achieved, there are far more effective methods available for installing kernel mode hooks, and this method should be avoided.

Another well-known Windows kernel hooking techniques is achieved via modification of the System Service Descriptor Table (SSDT). Like the IDT, the SSDT may be thought of as an array of function pointers that are invoked indirectly by user-mode calls into the kernel. Hooking the SSDT, then, involves simply modifying these function pointers to point to the entry point of a function of one's choosing. This hooking technique is more fine-grained than either of the techniques discussed thus far - the system defines an SSDT entry for each system service that it exposes. However, this technique remains extremely primitive and incredibly simple to detect. Furthermore, recent versions of Windows completely deny attempts at SSDT modification via Kernel Patch Protection (aka "PatchGuard"). Under kernel patch protection, various critical data structures within the Windows kernel are periodically checked for modification, and in the event that modification is detected, the system immediately bugchecks. Thus, like the methods discussed previously, this kernel hooking technique is more of a historical novelty rather than a practical control flow redirection mechanism.

A final method for kernel hooking involves modifying the major function table of a target driver. Every driver must define a major function table that contains pointers to routines that are invoked by the IO Manager whenever a request to a device managed by that driver is received. One may then hook the drivers major function table by replacing the pointers in the table with the address of a new function. Because this technique involves manipulation of dynamic kernel objects rather than static call tables, it is far more difficult to detect than any of the previously discussed methods. That said, there are still a large number of drivers on modern versions of Windows that monitor their own major function tables for modifications and bugcheck the system in the event that one is detected. An example of such a driver is the NTFS filesystem driver. Thus, one must still exercise caution when using this technique.