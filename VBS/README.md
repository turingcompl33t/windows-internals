## Virtualization-Based Security

Stuff and things!

### VBS Overview

**What is VBS?**

- Windows leveraging the hypervisor (Hyper-V) to provide security guarantees beyond what the kernel can provide on its own
- Consists of the following components
    - Device Guard -> stronger code signing guarantees through Hypervisor Code Integrity (HVCI)
    - Hyper Guard -> protected critical data structures and code, like Patch Guard?
    - Credential Guard -> protects unauthorized access to credentials, protects the lsass.exe process (by running it as a trustlet in IUM?)
    - Application Guard -> stronger sandbox for Edge
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

- If a change to memory page protection is requested in VTL 0, it must go through VTL 1 HVCI and be checked against the policy there
