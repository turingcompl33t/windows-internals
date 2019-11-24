### Windows 10 VM Hosted on Hyper-V

Create a Windows 10 ISO via the [Media Creation Tool](https://www.microsoft.com/en-us/software-download/windows10).

Create a new virtual machine within Hyper-V Manager. See the **Troubleshooting** section below to address some "gotchas" that may complicate this process.

**Troubleshooting**

VM fails to boot from ISO
- This happens frequently with Generation 2 VMs, unsure of the root cause
- Create a new Generation 1 VM with the same settings and attempt to boot from the ISO

"Windows Cannot Find the Software License Terms" during OS Installation
- Reorder the IDE hardware configuration in Hyper-V Manager
    - DVD drive with ISO is 0
    - Hard drive with virtual hard disk is 1
    - etc.
- Totally ridiculous that this is a fix for this issue

Windbg Preview Fails to Connect to Kernel over Serial Connection
- I frequently make the mistake of specifying the name of the COM port as the name of the pipe in the Windbg Preview interface
- e.g. if the VM is utilizing COM1 and mapping it to a named pipe, I would attempt to connect to a kernel at `\\.\COM1`
- Of course this fails to connect because this is not the actual named pipe identifier
- Instead: `\\.\pipe\COM1`

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

### VirtualKd-Redux

VirtualKD Redux is, as the name implies, an updated and modernized version of the original VirtualKD.

Download the latest release from the Github releases page [here](https://github.com/4d61726b/VirtualKD-Redux/releases).

**Guest (debug target) Setup Instructions:**

1. Copy the appropriate target folder to the guest
    - `target32/` for 32-bit target system
    - `target64/` for 64-bit target system
2. Run `vminstall.exe`
    - If running a Windows 10 VM, ensure "replace kdcom.dll" is checked
3. Restart the guest VM
4. At boot manager prompt, ensure VKD-Redux is selected, and press F8 for advanced boot options
5. Select "Disable Driver Signature Enforcement" and proceed with boot

This final step involving disabling driver signature enforcement is required each time the guest is booted to utilize VirtualKD Redux. Therefore, to save time in future kernel debugging iterations, it is recommended to make a snapshot of the guest VM at this point, once the target successfully boots.

**Host (debuggee) Setup Instructions:**

1. Run `vmmon.exe`
2. Ensure the debugger path is set correctly
3. Select "Run Debugger"