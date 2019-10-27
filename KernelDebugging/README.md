### VMWare and Hyper-V Interoperability

By default (prior to 2020) VMWare Workstation is not compatible with Hyper-V. There are a number of ways to circumvent this limitation.

1. Migrate VMs to Hyper-V. Stop using VMWare Workstation to host VMs.

2. Disable Hyper-V to run VMWare. Re-enable upon completion.

```
C:\>bcdedit /set hypervisorlaunchtype off
C:\>bcdedit /set hypervisorlaunchtype auto
```

3. Set up a new boot configuration to boot with Hyper-V disabled. Boot into this configuration when you plan on utilizing VMWare Workstation to host VMs.

```
C:\>bcdedit /copy {current} /d "Hyper-V Disabled" 
The entry was successfully copied to {ff-23-113-824e-5c5144ea}. 

C:\>bcdedit /set {ff-23-113-824e-5c5144ea} hypervisorlaunchtype off 
The operation completed successfully.
```

### VirtualKD

VirtualKD is a tool that improves kernel debugging performance for VMWare and VirtualBox.

Find the product [here](http://sysprogs.com/legacy/virtualkd/).