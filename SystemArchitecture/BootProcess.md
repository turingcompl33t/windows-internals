## Windows Boot Process

At the most general level, the Windows boot process begins with the Windows boot manager being loaded into memory and executed. However, the manner in which this takes place depends on the firmware environment present on the system, which is determined by the hardware installed on the motherboard:

- Legacy BIOS firmware
- Unified Extensible Firmware Interface (UEFI) firmware

### BIOS Firmware Boot

1. System is powered on
2. Power-On Self Test (POST) is performed, low level hardware checks
3. BIOS searches internal list of bootable devices for a boot sector
    - e.g. if the bootable device is a hard drive, this boot sector is a Master Boot Record (MBR)
4. BIOS transfers control to the initial bootcode in the identified boot sector
5. Bootcode from the MBR searches the partition table (also in the MBR) for the _active partition_ (also known as the _system volume_)
6. Bootcode from MBR transfers control to the bootcode in the Volume Boot Record (VBR) from the active partition
    - the VBR for the active partition consists of the first sector of the volume, similar to the MBR
7. Bootcode in the VBR reads the partition filesystem to locate a 16-bit _boot manager_ program
    - this boot manager program is located at _\%SystemDrive\bootmgr_
    - the boot manager program is actually two (2) distinct executables grafted into one, the first part being a 16-bit stub and the second part being either of 32-bit or 64-bit machine code (depending on the architecture) that actually implements the boot manager
8. Bootcode in the VBR transfers control to the boot manager stub
9. The boot manager stub, operating in 16-bit real mode, sets up necessary data structures, switches the processor into protected mode, and loads the remainder of the boot manager code into memory
10. The boot manager stub transfers control to the boot manager proper

The bootcode on the active partition (i.e. the system partition) is referred to as _ntfsboot.com_ by MS internally.

The two halves of the Windows boot manager (initially):

- _diskboot.com_ 16-bit stub at the front of the boot manager
- _bootmgr.exe_ a compressed PE image, either 32-bit or 64-bit code, decompressed by the _diskboot.com_ stub

Boot manager is loaded into memory a total of three (3) times during this initial boot procedure. This is obviously inefficient, and one among the many motivations for re-architecting the boot process.

All of the BIOS services, INT 13 disk services, for example, run in 16-bit real mode. Thus, even when the OS loader _winload.exe_ is running, it is constantly having to transition the processor mode in order to make use of these services. 

Another motivation for transition to EFI/UEFI firmware is the POST time required for legacy BIOS systems. Up to 20 seconds required for initial POST. The MS team felt that this was a poor user experience.

### EFI Firmware Boot

If the system is configured with EFI firmware, the process to transfer control to the Windows boot manager is vastly simplified. The Windows installer adds an EFI variable to the firmware specifying the path to the EFI application that implements the boot manager:

```
%SystemDrive%\EFI\Microsoft\Boot\Bootmgfw.efi
```

The EFI firmware switches the processor into protected mode, using a flat memory model with paging disabled, and subsequently transfers control to the Windows boot manager directly.

The UEFI boot manager looks at the following NVRAM variables in order to determine where it should transfer control:

- `BootNext` -> a one-time setting
- `BootOrder` -> a table of 16-bit values identifying OS boot managers (e.g. _bootmfgw.efi_)

The new Windows boot manager, _bootmgfw.efi_, sits on a FAT32 formatted volume. This is dictated by the UEFI specification.

Things the UEFI boot manager passes to the Windows boot manager:
- Image Handle -> "a handle to ourself" the OS boot manager
- System Table
    - Boot Services
    - Runtime Services

The facilities available in the boot services table replace the old BIOS services like INT 13. 

Networking services are available in the boot environment. This enables things like the network key protector unlock feature. 

The final UEFI boot service invoked by the Windows boot loader is `ExitBootServices()`
- This tells UEFI that the OS will now own the system

### The Windows Boot Manager

Both the BIOS firmware procedure and the EFI firmware procedure eventually load the Windows boot manager into memory and execute it. Configuration for the boot manager is stored in one of two locations on the system, depending on the firmware type:

- `%SystemDrive&\Boot` for BIOS systems
- `%SystemDrive%\EFI\Microsoft\Boot` for EFI systems

The boot configuration database will contain at least two (2) elements:

- A boot manager object
- One or more boot loader objects

### The Windows Boot Loader

The Windows boot loader is identified by one of the following two names, depending on the firmware type:

- _winload.exe_ on BIOS systems
- _Winload.efi_ on EFI systems

After performing some integrity checks both on its own image and on the kernel itself, the boot loader loads the kernel (_ntoskrnl.exe_), the Hardware Abstraction Layer (_hal.dll_), and the other DLLs upon which the kernel immediately depends.

Next, the boot loader identifies _boot start_ drivers under the registry key: `HKLM\SYSTEM\CurrentControlSet\Services`.

Finally, the boot loader transfers control to _ntoskrnl.exe_.

### Kernel and Executive Initialization

Once control has been handed over to the kernel, the real work of initializing the system begins.

- `KiSystemStartup()` serves as the kernel entry point
- Memory manager builds page tables
- HAL configures the interrupt controller for each processor, populates IVT, enables interrupts, etc.
- SSDT is constructed
- _ntdll.dll_ is loaded
- etc...

### Session Management

One of the final actions performed during executive initialization is session manager initialization. That is, the session manager program, _smss.exe_, is started.

The Windows subsystem consists of two (2) distinct parts, each of which must be initialized before the Windows subsystem is fully functional:

- The kernel mode driver _win32k.sys_
- The user mode component _csrss.exe_

The initial instance of _smss.exe_ actually ends up creating two (2) new instances of itself to manage system sessions:

- Session 0: hosts the init process _wininit.exe_
- Session 1: hosts the logon process _winlogon.exe_

### The Init Process, _wininit.exe_

The init process spawns three (3) child processes:

1. The Local Security Authority Subsystem _lsass.exe_: sits in a loop waiting for security-related requests via LPC
2. The Service Control Manager _services.exe_: loads and starts services
3. The Local Session Manager _lsm.exe_: handles new connections via terminal services

### The Logon Process, _winlogon.exe_

As the name implies, the logon process handles interactive user logons. It initializes the _logonui.exe_ process which passes credentials entered by a user to _lsass.exe_ for authentication.

In the event of a successful logon, _winlogon.exe_ starts up the configured programs for the particular user that has authenticated. These programs are configured via the following registry entries:

- `HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\UserInit`
- `HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\Shell`

The default value for the `UserInit` key, _userinit.exe_, is the one responsible for initializing the user session by, for example, cycling through various autorun keys in the registry like `Run` and `RunOnce`.

The default value for the `Shell` key, _explorer.exe_, serves as the parent process for all user mode applications started by the user in this session.

### VBS Boot: High-Level Overview

- Bootmgr.efi is validated by UEFI firmware and executed 
- Bootmgr.efi loads and validates winload.efi 
- Winload.efi loads and checks the VBS configuration
- Winload.efi loads and checks Hyper-V and both secure and unsecure kernel images
- Winload.efi exits EFI mode, hands control to Hyper-V