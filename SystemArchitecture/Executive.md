## Windows Executive

The Windows Executive implements base OS services - it implements constructs at a higher level of abstraction than the Windows Kernel proper.

### Overview 

The Windows Executive comprises the "upper layer" of the Windows kernel, _ntoskrnl.exe_. The Executive implements the majority of the core operating system services, including the following OS subsystems that implement the major logical system components:
- Configuration Manager: implements and manages the Windows registry
- Process Manager: manages the creation and destruction of Windows processes and threads; while the low-level support for process and thread management is provided by the Windows Kernel (distinguish the logical component from the executable image via capital "K" Kernel vs lowercase "k" kernel), the Executive adds an additional layer of abstraction on top of these APIs
- Security Reference Monitor (SRM): performs runtime access control checks and auditing in order to implement the Windows object-based security model
- IO Manager: manages all system IO operations; serves as the intermediary between user-mode IO requests and the kernel device drivers that service these requests
- Plug and Play Manager: implements support for dynamic driver discovery, loading, and unloading
- Power Manager: implements support for power state event notifications
- Memory Manager: implements the Windows virtual memory model 
- Cache Manager: implements Windows' support for software-based filesystem caching
- Object Manager: implements the Windows object model for the definition and export of system resources

Each of these Executive subsystems implement a set of functions that expose various portions of the functionality implement by that subsystem. A subset of these functions are exported and callable from user-mode applications via system service calls, while others are only available to kernel-mode callers such as device drivers.

### Executive Component Function Prefixes

The general format for all Windows system routines is:

```
<Prefix><Operation><Object>
```

For example:

```
ExAllocatePoolWithTag
KeInitializeThread
...
```

The table below lists the function prefixes for executive components along with component to which they below. The prefix may have the character _i_ in place of its listed final character to denote the fact that the routine is _internal_, or, equivalently, may have the character _p_ appended to the end of the complete prefix to denote the fact that the routine is _private_.

An example of the former: `Ki` denotes internal kernel routines.

An example of the latter: `Psp` denotes private process support routines.

| Prefix | Component                           |
|--------|-------------------------------------|
| Alpc   | Advanced Local Procedure Calls      |
| Cc     | Common Cache                        |
| Cm     | Configuration Manager               |
| Dbg    | Kernel debug support                |
| Dbgk   | User debug support                  |
| Em     | Errata Manager                      |
| Etw    | Event Tracing for Windows           |
| Ex     | Executive support routines          |
| FsRtl  | Filesystem Runtime Library          |
| Hv     | Hive Library                        |
| Hvl    | Hypervisor Library                  |
| Io     | I/O Manager                         |
| Kd     | Kernel Debugger                     |
| Ke     | Kernel                              |
| Kse    | Kernel Shim Engine                  |
| Lsa    | Local Security Authority            |
| Mm     | Memory Manager                      |
| Nt     | NT system services                  |
| Ob     | Object Manager                      |
| Pf     | Prefetcher                          |
| Po     | Power Manager                       |
| PoFx   | Power Framework                     |
| Pp     | PnP Manager                         |
| Ppm    | Processor Power Manager             |
| Ps     | Process Support                     |
| Rtl    | Runtime Library                     |
| Se     | Security Reference Monitor          |
| Sm     | Store Manager                       |
| Tm     | Transaction Manager                 |
| Ttm    | Terminal Timeout Manager            |
| Vf     | Driver Verifier                     |
| Vsl    | Virtual Secure Mode Library         |
| Wdi    | Windows Diagnostic Infrastructure   |
| Wfp    | Windows Fingerprint                 |
| Whea   | Windows Hardware Error Architecture |
| Wmi    | Windows Management Instrumentation  |
| Zw     | NT system services (mirror Nt)      |

