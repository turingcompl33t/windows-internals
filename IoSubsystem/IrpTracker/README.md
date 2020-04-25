### Exercise: Tracking `IRP`s

This exercise examines the manner in which the Windows IO Manager implements one component of its asynchronous exection model - how it manages IO Request Packets for pending asynchronous requests. It turns out that the IO Manager implements this functionality in two distinct ways depending on whether the IO operation is _thread-specific_ or _thread-agnostic_, and this exercise will demonstrate the precise implications of this distinction. 

### Setup and Configuration

This exercise requires a complete kernel debugging setup between a host system and a virtual machine running a recent version of Windows (Windows 10 works best). The OS instance running within the virtual machine will hereafter be referred to as the _debug target_. 

The `IrpTracker/` directory contains a Visual Studio solution that implements a kernel driver and a corresponding client application that are both required for this exercise. Build both of these executables with the _Build Solution_ option from within visual studio. Once you have built both executables, copy them to the debug target (this may be accomplished in a number of ways, such as via a shared folder). 

Finally, once both the driver (`IrpTrackerDriver.sys`) and the client application (`IrpTrackerClient.exe`) are present on the debug target, install and load the driver. There are various ways to accomplish this, but perhaps the simplest is with the help of the OSR Driver Loader application, available for download from our friends at OSR [here](http://www.osronline.com/article.cfm%5Earticle=157.htm).

Once the driver is installed and loaded successfully, you are ready to go.

### Procedure

The start of this procedure assumes that you have transferred both the client application `IrpTrackerClient.exe` and the support driver `IrpTrackerDriver.sys` to the debug target and have installed and loaded the driver successfully.

Launch the client application from a command prompt; elevation is not required. The first thing the client application does is acquire a handle to the device object created by the support driver. The source that accomplishes this is the following:

```
	auto device = ::CreateFileW(
		L"\\??\\IrpTracker",
		GENERIC_READ |
		GENERIC_WRITE,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		nullptr);
```

There are a few parameters of interest in this call to `CreateFile()`. The first is the name of the file that we specify - `\??\IrpTracker` - which implies the device created by the support driver is accessible in the global object manager namespace (this namespace is presented in Winobj as `GLOBAL??`). The support driver actually accomplishes this via the creation of a symbolic link at the time it is loaded, but we will explore this topic more in the next module on driver development. The other parameter of interest is the `FILE_FLAG_OVERLAPPED` flag which will allow us to issue asynchronous IO requests to the support driver in the same way that we are used to doing for regular files, Windows sockets, pipes, etc.

This call to `CreateFile()` returns a handle to a file object that represents an open instance of the device managed by the support driver. When it succeeds, you will be prompted to start the first IO operation.

```
> IrpTrackerClient.exe
[+] Acquired handle to driver device
[+] ENTER to issue thread-specific IO operation
```

Before initiating this first thread-specific IO operation, lets first look at the current state of our process and its outstanding IO operations. Break into the kernel debugger and locate the process hosting our instance of the client application.

```
kd> !process 0 2 IrpTrackerClient.exe
PROCESS ffffc38e2c0dc080
    SessionId: 1  Cid: 08fc    Peb: da309be000  ParentCid: 156c
    DirBase: 1bc32002  ObjectTable: ffffa08f5ad26540  HandleCount:  41.
    Image: IrpTrackerClient.exe

        THREAD ffffc38e3d0e9080  Cid 08fc.0a6c  Teb: 000000da309bf000 Win32Thread: 0000000000000000 WAIT: (Executive) KernelMode Alertable
            ffffc38e2ff24eb8  NotificationEvent
```

Here we use a detail level of `2` as the second argument to `!process` so that we get a list of the threads that are currently active in the process but not too much else - the address of the `ETHREAD` for our program's sole thread (at this point) is the real goal here. Once we have the address of our primary thread's `ETHREAD` object, we can use it to list thread-specific details.

```
kd> !thread ffffc38e3d0e9080 
THREAD ffffc38e3d0e9080  Cid 08fc.0a6c  Teb: 000000da309bf000 Win32Thread: 0000000000000000 WAIT: (Executive) KernelMode Alertable
    ffffc38e2ff24eb8  NotificationEvent
IRP List:
    ffffc38e3d0bc010: (0006,03a0) Flags: 00060900  Mdl: ffffc38e3b905880
    ...
```

The `!thread` command provides precisely the information in which we are interested - the IRPs that are pending for the current thread. But we notice something strange: we have yet to initate any IO operations, and yet we have a pending IRP queued to our primary thread. What is this IRP doing here? We can do some light detective work to figure it out:

```
1: kd> !irp ffffc38e3d0bc010 1
Irp is active with 2 stacks 1 is current (= 0xffffc38e3d0bc0e0)
 Mdl=ffffc38e3b905880: No System Buffer: Thread ffffc38e3d0e9080:  Irp stack trace.  
Flags = 00060900
ThreadListEntry.Flink = ffffc38e3d0e9700
ThreadListEntry.Blink = ffffc38e3d0e9700
    ...
Tail.CompletionKey = 116b080a350
     cmd  flg cl Device   File     Completion-Context
>[N/A(f), N/A(6)]
            0  1 00000000 00000000 00000000-00000000    pending

			Args: ffffa08f58fd3a70 116b2963c70 0x0 ffffc38e3b905880
 [IRP_MJ_READ(3), N/A(0)]
            0  0 ffffc38e2cd58e00 ffffc38e2ff24e20 00000000-00000000    
	       \Driver\condrv
			Args: 00001000 00000000 00000000 00000000
```

The output is somewhat difficult to make sense of, but the salient features are the fact that this IRP is a pending read (`IRP_MJ_READ`) request that is being handled by the console driver (`\Driver\condrv`). Thus, we can make the educated guess that this IRP is actually part of the system that allows our console application to communicate with the console device that controls it. 

Now that we have determined the only pending IRP currently present in our primary thread's queue is the console driver request, we are ready to proceed with making our own request to the support driver. Resuming exacution of the debug target and pressing ENTER to issue the thread-specific IO operation yields the following prompt:

```
[+] Issue thread-specific IO operation
[+] ENTER to complete the operation
```

A thread-specific IO operation has been initiated. How does the IO subsystem keep track of such requests? A quick look at the details of our application's primary thread starts to reveal how this is accomplished:

```
kd> !thread ffffc38e3d0e9080 
THREAD ffffc38e3d0e9080  Cid 08fc.0a6c  Teb: 000000da309bf000 Win32Thread: 0000000000000000 WAIT: (Executive) KernelMode Alertable
    ffffc38e2ff24eb8  NotificationEvent
IRP List:
    ffffc38e3d311c50: (0006,03a0) Flags: 00060900  Mdl: ffffc38e2e860840
    ffffc38e3a85e480: (0006,0118) Flags: 00060000  Mdl: 00000000
    ...
```

There are now two IRPs pending on our application's primary thread. But which one is the IRP associated with the request we just initiated?

```
1: kd> !irp ffffc38e3d311c50 1
Irp is active with 2 stacks 1 is current (= 0xffffc38e3d311d20)
 Mdl=ffffc38e2e860840: No System Buffer: Thread ffffc38e3d0e9080:  Irp stack trace.  
Flags = 00060900
ThreadListEntry.Flink = ffffc38e3a85e4a0
ThreadListEntry.Blink = ffffc38e3d0e9700
    ...
Tail.CompletionKey = 116b080a350
     cmd  flg cl Device   File     Completion-Context
>[N/A(f), N/A(6)]
            0  1 00000000 00000000 00000000-00000000    pending

			Args: ffffa08f58fd3a70 116b2963c70 0x0 ffffc38e2e860840
 [IRP_MJ_READ(3), N/A(0)]
            0  0 ffffc38e2cd58e00 ffffc38e2ff24e20 00000000-00000000    
	       \Driver\condrv
			Args: 00001000 00000000 00000000 00000000
```

```
1: kd> !irp ffffc38e3a85e480 1
Irp is active with 1 stacks 1 is current (= 0xffffc38e3a85e550)
 No Mdl: No System Buffer: Thread ffffc38e3d0e9080:  Irp stack trace.  
Flags = 00060000
ThreadListEntry.Flink = ffffc38e3d0e9700
ThreadListEntry.Blink = ffffc38e3d311c70
    ...
Tail.CompletionKey = 00000000
     cmd  flg cl Device   File     Completion-Context
>[IRP_MJ_DEVICE_CONTROL(e), N/A(0)]
            5  1 ffffc38e383199f0 ffffc38e3d04ee40 00000000-00000000    pending
	       \Driver\IrpTrackerDriver
			Args: 00000000 00000000 0x80002003 00000000
```

The first is a new IRP that is performing the same operation as the one we saw before - reading from the console driver device. The second IRP in the list, however, appears to match our expectations perfectly; it is a device IO control (`IRP_MJ_DEVICE_CONTROL`) requets that is being handled by the support driver (`\Driver\IrpTrackerDriver`). The corresponding source in the client application that made this request is as follows:

```
::DeviceIoControl(
		device,
		static_cast<unsigned long>(
			IRP_TRACKER_IOCTL_ADD_PENDING),
		nullptr, 0u,
		nullptr, 0u,
		&bytes_out,
		&ov1
```

Here, `device` is the handle to the file object that refers to the device created by the support driver that was acquired previously, while the second parameter specifies`IRP_TRACKER_IOCTL_ADD_PENDING` as the IO control code of the request made to the driver.

Before moving on, lets dig just a little deeper into the mechanics of how the pending IRPs for IO request made by our application are maintained. We have seen that the `!thread` command lists the IRPs pending for the specified thread, but we can derive this information ourselves with just a little more work.

```
kd> dt nt!_ETHREAD ffffc38e3d0e9080 
   +0x000 Tcb              : _KTHREAD
    ...
   +0x680 IrpList          : _LIST_ENTRY [ 0xffffc38e`3d311c70 - 0xffffc38e`3a85e4a0 ]
   ...
```

The `ETHREAD` structure is large, but the information we are looking for is relatively easy to locate; the `ETHREAD`'s `IrpList` member is a `LIST_ENTRY` structure that maintains a linked-list of all of the IRPs that are currently pending for this thread. Each entry in this linked-list is an `IRP` structure that itself has an embedded `LIST_ENTRY`:

```
kd> dt nt!_IRP
   +0x000 Type             : Int2B
    ...
   +0x020 ThreadListEntry  : _LIST_ENTRY
   ...
```

We can quickly walk the linked-list of pending IRPs to verify that the IRP for the thread-specific IO request we initiated is there - it just requires a bit of math:

```
kd> dt nt!_ETHREAD ffffc38e3d0e9080 IrpList.Flink 
   +0x680 IrpList       :  [ 0xffffc38e`3d311c70 - 0xffffc38e`3a85e4a0 ]
      +0x000 Flink         : 0xffffc38e`3d311c70 _LIST_ENTRY [ 0xffffc38e`3a85e4a0 - 0xffffc38e`3d0e9700 ]
kd> dt nt!_IRP 0xffffc38e3d311c70-0x20 -l ThreadListEntry.Flink
ThreadListEntry.Flink at 0xffffc38e`3d311c50
---------------------------------------------
   +0x000 Type             : 0n6
    ...

ThreadListEntry.Flink at 0xffffc38e`3a85e480
---------------------------------------------
   +0x000 Type             : 0n6
    ...
kd> !irp 0xffffc38e3a85e480 1
Irp is active with 1 stacks 1 is current (= 0xffffc38e3a85e550)
 No Mdl: No System Buffer: Thread ffffc38e3d0e9080:  Irp stack trace.  
Flags = 00060000
ThreadListEntry.Flink = ffffc38e3d0e9700
ThreadListEntry.Blink = ffffc38e3d311c70
    ...
Tail.CompletionKey = 00000000
     cmd  flg cl Device   File     Completion-Context
>[IRP_MJ_DEVICE_CONTROL(e), N/A(0)]
            5  1 ffffc38e383199f0 ffffc38e3d04ee40 00000000-00000000    pending
	       \Driver\IrpTrackerDriver
			Args: 00000000 00000000 0x80002003 00000000
```

So we are able to locate the IRP from our thread-specific IO request in the `ETHREAD` structure's linked-list of pending IRPs. 

Now we're ready to move on to the next part of the investigation. Resume execution of the debug target and press ENTER to complete the pending IO operation we initiated previously. Once we do, we are presented with the prompt below:

```
[+] Enter to issue thread-agnostic IO operation
```

Don't hit ENTER again just yet. Let's first confirm that we succeeded in completing the pending request. By now this should be old hat:

```
kd> !thread ffffc38e3d0e9080 
THREAD ffffc38e3d0e9080  Cid 08fc.0a6c  Teb: 000000da309bf000 Win32Thread: 0000000000000000 WAIT: (Executive) KernelMode Alertable
    ffffc38e2ff24eb8  NotificationEvent
IRP List:
    ffffc38e3d311c50: (0006,03a0) Flags: 00060900  Mdl: ffffc38e37a63430
    ...
kd> !irp ffffc38e3d311c50 1
Irp is active with 2 stacks 1 is current (= 0xffffc38e3d311d20)
 Mdl=ffffc38e37a63430: No System Buffer: Thread ffffc38e3d0e9080:  Irp stack trace.  
Flags = 00060900
ThreadListEntry.Flink = ffffc38e3d0e9700
ThreadListEntry.Blink = ffffc38e3d0e9700
    ...
Tail.CompletionKey = 116b080a350
     cmd  flg cl Device   File     Completion-Context
>[N/A(f), N/A(6)]
            0  1 00000000 00000000 00000000-00000000    pending

			Args: ffffa08f58fd3a70 116b2963c70 0x0 ffffc38e37a63430
 [IRP_MJ_READ(3), N/A(0)]
            0  0 ffffc38e2cd58e00 ffffc38e2ff24e20 00000000-00000000    
	       \Driver\condrv
			Args: 00001000 00000000 00000000 00000000
```

So the IRP from our IO request that was previously pending is now gone and the only pending IRP that remains is the familiar request to the console driver - exactly as we expected.

Now we are ready to issue the second IO operation. This time, however, the IO request that we make to the support driver will be _thread-agnostic_. The client application accomplishes this by creating an IO completion port and associating the existing handle to the file object that represents our open instance of the support driver's device object with it. Subsequently, any IO operations that are initiated on the file object from our application will be performed in a thread-agnostic manner, and this has implications for how the IO subsystem manages the underlying IRPs that implement these requests. 

Resume the debug target and press ENTER to initiate the thread-agnostic IO operation.

```
[+] Issued thread-agnostic IO operation
[+] ENTER to complete the operation
```

As usual, don't press ENTER again just yet. Break back into the debug target in the debugger so we can examine the new state of the system. First we'll look in the usual place to determine if the new IRP is queued to the application's primary thread:

```
kd> !thread ffffc38e3d0e9080
THREAD ffffc38e3d0e9080  Cid 08fc.0a6c  Teb: 000000da309bf000 Win32Thread: 0000000000000000 WAIT: (Executive) KernelMode Alertable
    ffffc38e2ff24eb8  NotificationEvent
IRP List:
    ffffc38e3d311c50: (0006,03a0) Flags: 00060900  Mdl: ffffc38e2e84b570
kd> !irp ffffc38e3d311c50 1
Irp is active with 2 stacks 1 is current (= 0xffffc38e3d311d20)
 Mdl=ffffc38e2e84b570: No System Buffer: Thread ffffc38e3d0e9080:  Irp stack trace.  
Flags = 00060900
ThreadListEntry.Flink = ffffc38e3d0e9700
ThreadListEntry.Blink = ffffc38e3d0e9700
    ...
Tail.CompletionKey = 116b080a350
     cmd  flg cl Device   File     Completion-Context
>[N/A(f), N/A(6)]
            0  1 00000000 00000000 00000000-00000000    pending

			Args: ffffa08f58fd3a70 116b2963c70 0x0 ffffc38e2e84b570
 [IRP_MJ_READ(3), N/A(0)]
            0  0 ffffc38e2cd58e00 ffffc38e2ff24e20 00000000-00000000    
	       \Driver\condrv
			Args: 00001000 00000000 00000000 00000000
```

The only IRP pending on the application's primary thread is the familiar read request to the console driver, so our new request is not here. Are there any new thread's in the process to which our IRP may be queued?

```
kd> !process 0 2 IrpTrackerClient.exe
PROCESS ffffc38e2c0dc080
    SessionId: 1  Cid: 08fc    Peb: da309be000  ParentCid: 156c
    DirBase: 1bc32002  ObjectTable: ffffa08f5ad26540  HandleCount:  45.
    Image: IrpTrackerClient.exe

        THREAD ffffc38e3d0e9080  Cid 08fc.0a6c  Teb: 000000da309bf000 Win32Thread: 0000000000000000 WAIT: (Executive) KernelMode Alertable
            ffffc38e2ff24eb8  NotificationEvent

        THREAD ffffc38e3d3ee080  Cid 08fc.172c  Teb: 000000da309c1000 Win32Thread: 0000000000000000 WAIT: (WrQueue) UserMode Non-Alertable
            ffffc38e3e5be3c0  QueueObject
```

So there is a new thread that has been created (the latter in this output, you can determine the new thread in your own output by comparing the address of the `ETHREAD` structures this command reports with that of the primary thread we have been using). The client application created this thread to handle IO completions associated with the IO completion port, is the IRP for our new IO request queued to this new thread?

```
kd> !thread ffffc38e3d3ee080
THREAD ffffc38e3d3ee080  Cid 08fc.172c  Teb: 000000da309c1000 Win32Thread: 0000000000000000 WAIT: (WrQueue) UserMode Non-Alertable
    ffffc38e3e5be3c0  QueueObject
Not impersonating
    ...
```

No dice. This new thread has no pending IRPs queued to it. So how is the IO subsystem tracking the IO request we just made... has it simply been lost? 

Of course not, but to find it we will have to do a bit more digging. The first step is to locate the handle to the `FILE_OBJECT` that represents our process' open instance of the support driver device object. The `!handle` command makes this relatively simple:

```
kd> !handle 0 3 ffffc38e2c0dc080 File
```

The address provided to `!handle` is the address of the `EPROCESS` for the client application instance while the `File` argument limits the output of the command to handles that refer file objects. However, the command still produces a decent amount of output:

```
1: kd> !handle 0 3 ffffc38e2c0dc080 File

Searching for handles of type File

PROCESS ffffc38e2c0dc080
    SessionId: 1  Cid: 08fc    Peb: da309be000  ParentCid: 156c
    DirBase: 1bc32002  ObjectTable: ffffa08f5ad26540  HandleCount:  45.
    Image: IrpTrackerClient.exe

Handle table at ffffa08f5ad26540 with 45 entries in use

0004: Object: ffffc38e2ff236b0  GrantedAccess: 0012019f Entry: ffffa08f5b1ff010
Object: ffffc38e2ff236b0  Type: (ffffc38e2c0fcf00) File
    ObjectHeader: ffffc38e2ff23680 (new version)
        HandleCount: 2  PointerCount: 65536
        Directory Object: 00000000  Name: \Reference {ConDrv}

0044: Object: ffffc38e2ff24e20  GrantedAccess: 0012019f (Protected) (Inherit) (Audit) Entry: ffffa08f5b1ff110
Object: ffffc38e2ff24e20  Type: (ffffc38e2c0fcf00) File
    ObjectHeader: ffffc38e2ff24df0 (new version)
        HandleCount: 2  PointerCount: 65456
        Directory Object: 00000000  Name: \Input {ConDrv}

    ...

004c: Object: ffffc38e3d03dcd0  GrantedAccess: 00100020 (Inherit) Entry: ffffa08f5b1ff130
Object: ffffc38e3d03dcd0  Type: (ffffc38e2c0fcf00) File
    ObjectHeader: ffffc38e3d03dca0 (new version)
        HandleCount: 1  PointerCount: 32768
        Directory Object: 00000000  Name: \IrpTracker {HarddiskVolume4}

    ...

00a4: Object: ffffc38e3d04ee40  GrantedAccess: 0012019f (Protected) Entry: ffffa08f5b1ff290
Object: ffffc38e3d04ee40  Type: (ffffc38e2c0fcf00) File
    ObjectHeader: ffffc38e3d04ee10 (new version)
        HandleCount: 1  PointerCount: 32766
```

The second-to-last entry in the output above looks promising - it has `\IrpTracker` listed after the `Name` identifier. However, don't be fooled by this entry. The one we are looking for is actually the last entry in the above output (and likely the last entry in the output produced when you run the command as well) despite the fact that no identifying information is provided. Now that we have the address of the file object, we can use the `!fileobj` command to display more details about it:

```
1: kd> !fileobj ffffc38e3d04ee40
```

Here, the address provided as the address to `!fileobj` is the address that follows the `Object` identifier in the output from `!handle`. 

```
1: kd> !fileobj ffffc38e3d04ee40



Device Object: 0xffffc38e383199f0   \Driver\IrpTrackerDriver
Vpb is NULL

Flags:  0x40000
	Handle Created

IRPs queued to File Object:

	ffffc38e2c9f7950   \Driver\IrpTrackerDriver

1 queued.

CurrentByteOffset: 0
```

The output from `!fileobj` gives us exactly what we are looking for - it reports that there is a single IRP queued to the `FILE_OBJECT` itself, and that this IRP is being managed by the support driver (`\Driver\IrpTrackerDriver`). However, just as we did with the `EPROCESS` structure in the thread-specific IO case, we can dig just a little deeper to see exactly how this is accomplished.

```
1: kd> dt nt!_FILE_OBJECT ffffc38e3d04ee40
   +0x000 Type             : 0n5
   +0x002 Size             : 0n216
    ...
   +0x0b8 IrpListLock      : 0
   +0x0c0 IrpList          : _LIST_ENTRY [ 0xffffc38e`2c9f7970 - 0xffffc38e`2c9f7970 ]
   +0x0d0 FileObjectExtension : (null) 
```

So the `FILE_OBJECT` has an embedded `LIST_ENTRY` structure that maintains a linked-list of queued `IRP`s. 

```
kd> dt nt!_FILE_OBJECT ffffc38e3d04ee40 IrpList.Flink
   +0x0c0 IrpList       :  [ 0xffffc38e`2c9f7970 - 0xffffc38e`2c9f7970 ]
      +0x000 Flink         : 0xffffc38e`2c9f7970 _LIST_ENTRY [ 0xffffc38e`3d04ef00 - 0xffffc38e`3d04ef00 ]
kd> !irp 0xffffc38e2c9f7970-0x20
Irp is active with 1 stacks 1 is current (= 0xffffc38e2c9f7a20)
 No Mdl: No System Buffer: Thread ffffc38e3d0e9080:  Irp stack trace.  
     cmd  flg cl Device   File     Completion-Context
>[IRP_MJ_DEVICE_CONTROL(e), N/A(0)]
            5  1 ffffc38e383199f0 ffffc38e3d04ee40 00000000-00000000    pending
	       \Driver\IrpTrackerDriver
			Args: 00000000 00000000 0x80002003 00000000
```

Where did that `- 0x20` come from? We saw before that the offset of the `ThreadListEntry` field within the `IRP` structure is `0x20` and it happens that for IRPs that are queued to file objects, this field is reused to implement the linked-list functionality.

Finally, complete the exercise by resuming the debug target and pressing ENTER to complete the second request. On successful termination, you should be greeted by:

```
[+] Success; exiting
```

