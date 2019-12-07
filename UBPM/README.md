## Unified Background Process Manager (UBPM)

The Unified Background Process Manager (UBPM) is, as the name implies, a unified scheduling engine for background tasks. Historically, various processes across the system have been charged with managing background tasks, including:

- Service Control Manager -> manages services
- Task Scheduler -> manages Windows Tasks
- WMI -> manages WMI providers

The goal of UBPM is to expose a common API and implement a centralized system for managing background process lifecycles.

### UBPM Implementation

The UBPM is implemented in _Services.exe_ - the same binary that hosts the SCM. It is, however, completely separate from the SCM and the SCM must still interact with UBPM via the public, exported API (_Ubpm.dll_).

The primary mechanism by which the UBPM performs its job is through ETW tracing. Upon initialization, it registers an ETW consumer, starts a realtime session (in secure mode),  and registers a callback function to handle event notifications.

The UBPM sets up the first ETW provider to listen for notifications related to time changes. Once initilization is complete, services may register further providers via the API exposed by the extension DLL loaded by _Services.exe_.

The SCM and Task Scheduler register consumers for these provider events in a similar manner.

### References

- _Windows Internals, 6th Edition Part 1_ Pages 336-342