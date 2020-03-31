## Minimal Processes and Pico Processes

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

### What are the properties of minimal processes?

- No executable image file associated with the process
- User mode virtual address space is untouched, not managed by the system
- No initial thread created
- No TEBs created when a new thread is initialized in the process, because all threads created in this process will be “minimal threads”
- No PEB
- _ntdll.dll_ not mapped in the process virtual address space
- Shared user data section not mapped in the process virtual address space

### How are minimal processes utilized?

- System process is a minimal process, just a container for system threads
- Minimal process created to indicate to management tools (e.g. _taskmgr.exe_) that VBS is running
- Memory compression uses address space of a minimal process to do its thing

### Further Reading

- [Microsoft Drawbridge](https://www.microsoft.com/en-us/research/project/drawbridge/?from=http%3A%2F%2Fresearch.microsoft.com%2Fen-us%2Fprojects%2Fdrawbridge%2F)
- [Channel9: Drawbridge: A New form of Virtualization for Application Sandboxing](https://channel9.msdn.com//shows//going+Deep//drawbridge-An-Experimental-Library-Operating-System)