### User Mode Processes

- User Processes: standard user applications, 16, 32, or 64 bit
    - ex: notepad.exe
- Service Processes: processes that host Windows services, which typically means they may exist outside of any single user logon session
    - ex: task scheduler
- System Processes: fixed processes that are NOT managed by the SCM
    - ex: lsass.exe
- Environment Subsystem Server Processes: implement the support for the OS subsystem presented to the user
- ex: csrss.exe
 
### Core Subsystem DLLs

- kernel32.dll: exposes most of the Win32 base APIs
- advapi32.dll: security calls and functions for maniuplating the registry
- user32.dll: the user component of Windows GUI rendering and window events
- gdi32.dll: graphics device interface, low level primitives for rendering graphics
 
### System Support Library (ntdll.dll)

- Exports the "native API"
- The lowest level OS interface available to user mode
- Responsible for issuing system calls
- Supports native applications (those with no subsystem)
- Also used extensively by programs that run in user mode early during system initialization
    - e.g. csrss.exe implements the Windows subsystem, and thus cannot rely on the Windows subsystem, so it must be a native application that utilizes the native API directly
 
### Windows Subsystem

- Implemented by csrss.exe
- "Client server runtime subsystem"
 
### Windows Subsystem for Linux (WSL)

- Version 1 -> uses pico processes and pico providers
- Version 2 -> uses the hypervisor

### What is a minimal process?

- Originated as a result of interest in implementing pico processes
- Pico processes allowed for lightweight virtualization solutions
    - E.g. run an XP application on Windows 10
- Accomplished by essentially running a stripped-down XP OS in userland on the Windows 10 system, with the XP application on top of that 
- Importantly, there is no need for a virtual machine here, both the XP OS and application are running in a single process on the Windows 10 host
- Pico processes required the host OS to get out of the way and not try to manage the address space of the pico process
- A minimal process is the most rudimentary process, it just tells the host OS to get out of the way and not try to manage it 
- From host OS perspective, it is just an empty VAS
- A pico process is then a minimal process which is serviced by a pico provider running in the kernel 

Minimal process properties
- No executable image file associated with the process
- User mode VAS is untouched
- No initial thread created
- No TEBs created when a new thread is initialized in the process, because all threads created in this process will be “minimal threads”
- No PEB
- Ntdll.dll not mapped
- Shared user data section not mapped

Where are minimal processes used?
- System process is a minimal process, just a container for system threads
- Minimal process created to indicate to management tools (e.g. taskmgr.exe) that VBS is running
- Memory compression uses address space of a minimal process to do its thing

### Virtualization Based Security (VBS)

What is VBS?
- Windows leveraging the hypervisor (Hyper-V) to provide security guarantees beyond what the kernel can provide on its own
- Consists of the following components
    - Device Guard -> stronger code signing guarantees through Hypervisor Code Integrity (HVCI)
    - Hyper Guard -> protected critical data structures and code, like Patch Guard?
    - Credential Guard -> protects unauthorized access to credentials, protects the lsass.exe process (by running it as a trustlet in IUM?)
    - Application Guard -> stronger sandbox for Edge
    - Host Guardian and Shielded Fabric -> use virtual TPM to protect a VM from the infrastructure it runs on

How does VBS work?
- Virtual trust levels (VTLs) that operate orthogonally to processor privilege levels (rings)
Privilege levels enforce power, VTLs enforce isolation
- VTL 0 -> normal user / kernel modes
- VTL 1 -> isolated user mode (IUM) / secure kernel mode (SKM)
- Separate binaries
- Secure kernel = securekenel.exe
- Secure ntdll.dll = iumdll.dll, limits system calls, adds new “secure” system calls only available in VTL 1
- Windows subsystem = iumbase.dll 
- Only specially signed binaries, trustlets, are allowed to execute in VTL 1

Credential Guard
- Isolates the lsass.exe process by running it in IUM as lsaiso.exe

Device Guard
- If a change to memory page protection is requested in VTL 0, it must go through VTL 1 HVCI and be checked against the policy there
