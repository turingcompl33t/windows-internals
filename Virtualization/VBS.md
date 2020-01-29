## Virtualization-Based Security

Stuff and things!

### VBS Overview

Virtualization-Based Security (VBS) is a new Windows security feature enabled by the Windows hypervisor (Hyper-V). Under VBS, the concept of a Virtual Trust Level (VTL) is introduced to construct a new security boundary that is orthogonal to the typical user-mode (Ring 3) / kernel-mode (Ring 0) distinction that has heretofore provided the primary system security boundary. 

In the new VBS model, the standard Windows kernel and user-mode run in VTL 0, still separated by the privilege-level distinction implemented by the processor and the memory manager. In addition to this, their is also a secure kernel and secure user-mode (called Isolated User Mode) that execute in the context of VTL 1. The partition between VTL 0 and VTL 1 is enforced by the Windows hypervisor with the help of two hardware mechanisms: second-level address translation (SLAT) and an IO Memory Management Unit (MMU). 

The secure kernel implements a very small subset of the functionality exposed by the standard Windows kernel. For this reason, processes that execute in isolated user mode (called trustlets) are typically far less powerful than their non-ioslated counterparts because they may only access a limited subset of the system's functionality. 

The purpose of Microsoft's introduction of VBS is to create a situation in which even a compromised kernel in VTL 0 does not result in complete compromise of the system. Because of the introduction of VTLs, even the kernel itself is limited in its power because it cannot access code or data running at a higher VTL, such as the secure kernel and isolated user mode processes. Thus, complete system compromise under VBS requires not only kernel-level access, but also compromise of the Windows hypervisor in order to gain access to the critical system functionalities that have been moved to the VTL 1 context. 

Microsoft has introduced a variety of specific security upgrades that rely upon the VBS infrastructure. Some of these technologies include:
- Credential Guard: under credential guard, the `lsass` process no longer stores user authentication data in its process address space; this task has been delegated to the `lsaiso` trustlet that exeutes in IUM at VTL 1
- Device Guard: device guard is an application whitelist technology that leverages VBS features
- Windows Defender Application Guard: under Windows Defender Application Guard, certain high-risk applications are executed within their own lightweight virtual machine (perhaps "container" is a more appropriate term) in an attempt to isolate them from the rest of the system; the newest version of the Edge browser is an example of an application that utilizes this technology 

This list is far from exhaustive, and Microsoft continues to roll out new security features that leverage VBS infrastructure at a rapid pace.

**What is VBS?**

- Windows leveraging the hypervisor (Hyper-V) to provide security guarantees beyond what the kernel can provide on its own
- Consists of the following components
    - Device Guard -> stronger code signing guarantees through Hypervisor Code Integrity (HVCI)
    - Hyper Guard -> protected critical data structures and code, like Patch Guard?
    - Credential Guard -> protects unauthorized access to credentials, protects the lsass.exe process (by running it as a trustlet in IUM?)
    - Application Guard -> stronger sandbox for Edge, and continually expanding to other processes that MS deems worthy
    - Host Guardian and Shielded Fabric -> use virtual TPM to protect a VM from the infrastructure it runs on

**How does VBS work?**

- Virtual trust levels (VTLs) that operate orthogonally to processor privilege levels (rings)
Privilege levels enforce power, VTLs enforce isolation
- VTL 0 -> normal user / kernel modes
- VTL 1 -> isolated user mode (IUM) / secure kernel mode (SKM)
- Separate binaries
- Secure kernel = securekenel.exe
- Secure _ntdll.dll_ = _iumdll.dll_, limits system calls, adds new “secure” system calls only available in VTL 1
- Windows subsystem = _iumbase.dll_ 
- Only specially signed binaries, trustlets, are allowed to execute in VTL 1

**Credential Guard**

- Isolates the _lsass.exe_ process by running it in IUM as _lsaiso.exe_

**Device Guard**

- If a change to memory page protection is requested in VTL 0, it must go through VTL 1 HVCI and be checked against the policy there.
