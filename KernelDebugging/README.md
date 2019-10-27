### Windows 10 VM Hosted on Hyper-V

Forthcoming.

### Windows 10 VM Hosted on VMWare 

Create a Windows 10 ISO via the [Media Creation Tool](https://www.microsoft.com/en-us/software-download/windows10).

Create a new virtual machine within VMWare; install the operating system from the ISO image file.

Configure a virtual serial port for the debugee VM within VMWare:

- Add Hardware -> Serial Port
    - Output to Named Pipe: "\\.\pipe\COM1"
    - Yield CPU on Poll
    - Connect at Power On
    - This End is Server
    - The Other End is a Virtual Machine
- Make a note of the serial port number assigned by VMWare, it may not always be "1" and this determines the boot configuration that we set up next

Make the necessary boot configuration changes within the debuggee VM:

```
bcdedit /copy {current} /d "Windows 10 [debugger disabled]"
bcdedit /debug {current} ON
bcdedit /dbgsettings {current} serial debugport:2 baudrate:115200 /noumex
```

- The first line makes a copy of the existing boot configuration, useful for if we ever need to roll back.
- Notice the `/debugport` argument on the last line, VMWare assigned the new serial port to number 2, so this is the number we must use here
- The final argument on the last line, `/noumex`, prevents user mode exceptions within the debugee VM from breaking into the kernel debugger. Omit if desired.

### VMWare and Hyper-V Interoperability

By default (prior to 2020) VMWare Workstation is not compatible with Hyper-V. There are a number of ways to circumvent this limitation.

1. Migrate VMs to Hyper-V. Stop using VMWare Workstation to host VMs.

2. Disable Hyper-V to run VMWare. Re-enable upon completion.

```
bcdedit /set hypervisorlaunchtype off
bcdedit /set hypervisorlaunchtype auto
```

3. Set up a new boot configuration to boot with Hyper-V disabled. Boot into this configuration when you plan on utilizing VMWare Workstation to host VMs.

```
bcdedit /copy {current} /d "Hyper-V Disabled" 
The entry was successfully copied to {ff-23-113-824e-5c5144ea}. 

bcdedit /set {ff-23-113-824e-5c5144ea} hypervisorlaunchtype off 
The operation completed successfully.
```

### VirtualKD

VirtualKD is a tool that improves kernel debugging performance for VMWare and VirtualBox.

Find the product [here](http://sysprogs.com/legacy/virtualkd/).

VirtualKD is legit for debugging kernels up through but not including Windows 8 / Server 2012. The tool seems to have trouble coping with driver integrity verification, even when we explicitly disable it in these newer OS versions. I have never been able to get the debugger to attach under these circumstances. 