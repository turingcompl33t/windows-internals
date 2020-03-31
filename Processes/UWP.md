## Universal Windows Platform (UWP) Processes

Properties of UWP processes include:

- Run within the context of an AppContainer application sandbox
- Runs under the control of the process lifetime manager (PLM) that controls the process state (e.g. foreground, background)
- Single instance by default
- Must have application _identity_, as provided by the application package
- Launched by the DCOM Launch Service process, hosted by an instance of _svchost.exe_

