## Windows Executive

The Windows Executive implements base OS services - it implements constructs at a higher level of abstraction than the Windows Kernel proper.

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

