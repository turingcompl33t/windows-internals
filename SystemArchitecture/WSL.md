## System Architecture: Windows Subsystem for Linux (WSL)

### Overview

Thus far, two distinct generations of WSL:

- WSL 1
- WSL 2

### WSL Architecture

WSL 1 and WSL 2 present the same interface (more or less) but differ significantly in their underlying architecture. At a high level, the largest distinction is that WSL 1 merely emulates native Linux system calls via an indirection layer (see below) while WSL 2 ships the entire Linux kernel with Windows and runs distributions on top of that in a virtualized environment.

**WSL 1 Architecture**

- Linux Distribution in Userspace -> provided by partners, not by MS
- Translation Layer -> implemented as a Windows driver
- NT Kernel 

**WSL 2 Architecture**

WSL 2 leverages MS virtualization technology. Specifically, it utilizes a "lightweight utility VM" in MS parlance. This type of VM was originally designed for server environments and poses relatively-low overhead for the host. This VM is distinct from the normal VM paradigm:

- It maintains a relatively high level of integration with the host system, rather than the traditional isolation we expect from a VM
- It is not constantly running, rather it runs on demand

The big takeaway from the architecture of WSL 2 is that the Windows hypervisor, Hyper-V, virtualizes both the Windows kernel and WSL VM instances in the same manner. Obviously there are distinctions between the privileges of these virtualized environments, but they fundamentally operate at the same level.

Another important note is that in the event that one is running multiple Linux distributions at the same time, these distros will run within the same lightweight utility VM on top of the same Linux kernel - we do not spin up a new VM instance for each running WSL distribution instance.

**Deep Dive**

- The entry point to WSL is _wsl.exe_ -> "the swiss army knife for interacting with WSL"
- This calls into Lxss Manager Service -> tracks installed distros, running distros, etc.
- In order to launch a new VM, the Lxss Manager Service calls into the Host Compute Service which launches the utility VM
- This VM hosts both the user space Linux distro as well as the Linux kernel
- The Linux distro now runs the Linux init process, bash, etc.

### Shipping a Linux Kernel

To implement WSL, MS ships a full Linux kernel with Windows. It is built from the same open-source code available at kernel.org, with tuning specific to the WSL use case.

### Filesystem Details

The filesystem implementation changed significantly between WSL1 and WSL2. In WSL1, all files, including Linux files, were still stored on an NTFS volume. In WSL2, this is no longer the case - Linux files are stored on a separate EXT4 volume. 

When we access a file from user space on Windows, the request is routed to a driver called the Multiple UNC Provider (MUP). This driver maintains a list of redirectors and determines the appropriate redirector to which the filesystem access request should be routed. Examples of redirectors include:

- Plan 9 Server
- RDP
- SMB

The Plan 9 file server is the redirector of interest for WSL.

The WSL init process (running as a pico process in userspace) hosts a Plan 9 server that opens up a UNIX socket to listen for requests destined for the Plan 9 redirector.

### References

- [Announcing WSL 2](https://devblogs.microsoft.com/commandline/announcing-wsl-2/)
- [The New Windows Subsystem for Linux Architecture](https://www.youtube.com/watch?v=lwhMThePdIo&t=2838s)
- [How WSL Accesses Linux Files from Windows](https://channel9.msdn.com/Shows/Going+Deep/How-WSL-accesses-Linux-files-from-Windows)
- [A Deep Dive into How WSL Allows Windows to Access Linux Files](https://devblogs.microsoft.com/commandline/a-deep-dive-into-how-wsl-allows-windows-to-access-linux-files/)
- [Shipping a Linux Kerne with Windows](https://devblogs.microsoft.com/commandline/shipping-a-linux-kernel-with-windows/)