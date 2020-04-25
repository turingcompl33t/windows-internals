
```
> IrpTrackerClient.exe
[+] Acquired handle to driver device
[+] ENTER to issue thread-specific IO operation
```

Press ENTER to issue the thread-specific IO operation. 

```
[+] Issue thread-specific IO operation
[+] ENTER to complete the operation
```

Look at the outstanding IRPs for our main thread.

```
kd> !process 0 2 IrpTrackerClient.exe
PROCESS ffff8d8f37916080
    SessionId: 1  Cid: 1370    Peb: 5a52216000  ParentCid: 0f3c
    DirBase: 6a4b3002  ObjectTable: ffffa500500605c0  HandleCount:  36.
    Image: IrpTrackerClient.exe

        THREAD ffff8d8f397e1080  Cid 1370.13c0  Teb: 0000005a52217000 Win32Thread: 0000000000000000 WAIT: (Executive) KernelMode Alertable
            ffff8d8f3e83b618  NotificationEvent
```

```
kd> !thread ffff8d8f397e1080
THREAD ffff8d8f397e1080  Cid 1370.13c0  Teb: 0000005a52217000 Win32Thread: 0000000000000000 WAIT: (Executive) KernelMode Alertable
    ffff8d8f3e83b618  NotificationEvent
IRP List:
    ffff8d8f4d8142b0: (0006,0238) Flags: 00060900  Mdl: ffff8d8f3fcab6a0
    ffff8d8f39239ea0: (0006,0118) Flags: 00060000  Mdl: 00000000
```

```
1: kd> !irp ffff8d8f39239ea0
Irp is active with 1 stacks 1 is current (= 0xffff8d8f39239f70)
 No Mdl: No System Buffer: Thread ffff8d8f397e1080:  Irp stack trace.  
     cmd  flg cl Device   File     Completion-Context
>[IRP_MJ_DEVICE_CONTROL(e), N/A(0)]
            5  1 ffff8d8f4c2ebb70 ffff8d8f4ac774f0 00000000-00000000    pending
	       \Driver\IrpTrackerDriver
			Args: 00000000 00000000 0x80002003 00000000
```

```
kd> dt nt!_ETHREAD ffff8d8f397e1080
    ...
   +0x680 IrpList          : _LIST_ENTRY [ 0xffff8d8f`4d8142d0 - 0xffff8d8f`39239ec0 ]
    ...
```

```
1: kd> dt nt!_IRP
   +0x000 Type             : Int2B
   +0x002 Size             : Uint2B
   +0x004 AllocationProcessorNumber : Uint2B
   +0x006 Reserved         : Uint2B
   +0x008 MdlAddress       : Ptr64 _MDL
   +0x010 Flags            : Uint4B
   +0x018 AssociatedIrp    : <anonymous-tag>
   +0x020 ThreadListEntry  : _LIST_ENTRY
   ...
```

```
kd> dt nt!_ETHREAD ffff8d8f397e1080 IrpList.Flink
   +0x680 IrpList       :  [ 0xffff8d8f`4d8142d0 - 0xffff8d8f`39239ec0 ]
      +0x000 Flink         : 0xffff8d8f`4d8142d0 _LIST_ENTRY [ 0xffff8d8f`39239ec0 - 0xffff8d8f`397e1700 ]
```

```
kd> dt nt!_IRP 0xffff8d8f4d8142d0-0x20 -l ThreadListEntry.Flink -y MdlAddress
ThreadListEntry.Flink at 0xffff8d8f`4d8142b0
---------------------------------------------
   +0x008 MdlAddress : 0xffff8d8f`3fcab6a0 _MDL
   +0x020 ThreadListEntry :  [ 0xffff8d8f`39239ec0 - 0xffff8d8f`397e1700 ]

ThreadListEntry.Flink at 0xffff8d8f`39239ea0
---------------------------------------------
   +0x008 MdlAddress : (null) 
   +0x020 ThreadListEntry :  [ 0xffff8d8f`397e1700 - 0xffff8d8f`4d8142d0 ]
```

```
kd> g
```

```
[+] Enter to issue thread-agnostic IO operation
```

```
0: kd> !thread ffff8d8f397e1080
THREAD ffff8d8f397e1080  Cid 1370.13c0  Teb: 0000005a52217000 Win32Thread: 0000000000000000 WAIT: (Executive) KernelMode Alertable
    ffff8d8f3e83b618  NotificationEvent
IRP List:
    ffff8d8f3969fd70: (0006,0238) Flags: 00060900  Mdl: ffff8d8f494e9cf0
```

```
0: kd> dt nt!_ETHREAD ffff8d8f397e1080
    ...
   +0x680 IrpList          : _LIST_ENTRY [ 0xffff8d8f`3969fd90 - 0xffff8d8f`3969fd90 ]
   ...
```

```
kd> g
```

```
[+] Issued thread-agnostic IO operation
[+] ENTER to complete the operation
```

```
0: kd> !thread ffff8d8f397e1080
THREAD ffff8d8f397e1080  Cid 1370.13c0  Teb: 0000005a52217000 Win32Thread: 0000000000000000 WAIT: (Executive) KernelMode Alertable
    ffff8d8f3e83b618  NotificationEvent
IRP List:
    ffff8d8f3969fd70: (0006,0238) Flags: 00060900  Mdl: ffff8d8f494e9cf0
```

```
0: kd> !handle 0 3 ffff8d8f37916080

PROCESS ffff8d8f37916080
    SessionId: 1  Cid: 1370    Peb: 5a52216000  ParentCid: 0f3c
    DirBase: 6a4b3002  ObjectTable: ffffa500500605c0  HandleCount:  38.
    Image: IrpTrackerClient.exe

Handle table at ffffa500500605c0 with 38 entries in use
    ...
0044: Object: ffff8d8f4ac76a00  GrantedAccess: 00100020 (Protected) (Audit) Entry: ffffa500533ff110
Object: ffff8d8f4ac76a00  Type: (ffff8d8f378fb0c0) File
    ObjectHeader: ffff8d8f4ac769d0 (new version)
        HandleCount: 1  PointerCount: 32768
        Directory Object: 00000000  Name: \TrackingIrps {HarddiskVolume4}]
    ...
    TODO actual one here!!!
```

```
kd> !fileobj ffff8d8f4ac774f0



Device Object: 0xffff8d8f4c2ebb70   \Driver\IrpTrackerDriver
Vpb is NULL

Flags:  0x40400
	Queue Irps to Thread
	Handle Created

IRPs queued to File Object:

	ffff8d8f4a3c5810   \Driver\IrpTrackerDriver

1 queued.

CurrentByteOffset: 0
```

```
kd> dt nt!_FILE_OBJECT ffff8d8f4ac774f0
    ...
   +0x0c0 IrpList          : _LIST_ENTRY [ 0xffff8d8f`4a3c5830 - 0xffff8d8f`4a3c5830 ]
   ...
```

```
0: kd> !irp 0xffff8d8f`4a3c5830-0x20
Irp is active with 1 stacks 1 is current (= 0xffff8d8f4a3c58e0)
 No Mdl: No System Buffer: Thread ffff8d8f397e1080:  Irp stack trace.  
     cmd  flg cl Device   File     Completion-Context
>[IRP_MJ_DEVICE_CONTROL(e), N/A(0)]
            5  1 ffff8d8f4c2ebb70 ffff8d8f4ac774f0 00000000-00000000    pending
	       \Driver\IrpTrackerDriver
			Args: 00000000 00000000 0x80002003 00000000
```





