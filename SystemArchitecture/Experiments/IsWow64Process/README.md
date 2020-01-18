## Experiment: `IsWoW64Process()`

In this experiment we make use of the Win32 API function `IsWoW64Process()` to introspect on the running program and determine or not it is subject to WoW64 interdiction, and furthermore look into how this function performs its query.

### System Information

This experiment was performed on a system with the following specifications:

```
OS Name:                   Microsoft Windows 10 Pro for Workstations
OS Version:                10.0.18362 N/A Build 18362
System Type:               x64-based PC
```

### Building the Test Application

We build two versions of the test application: one 32-bit version and one 64-bit version.

Build the 32-bit version of the application by running the following command from a x86 Native Tools command prompt:

```
cl /EHsc /nologo /Fe:Program86 Program.cpp
```

Similarly, build the 64-bit version of the application by running the following command from a x64 Native Tools command prompt:

```
cl /EHsc /nologo /Fe:Program64 Program.cpp
```

### Running the Test Application

Running the 32-bit version of the application produces the following output:

```
> Program86.exe
IsWoW64Process: TRUE
```

In contrast, running the 64-bit version of the application produces the following output:

```
> Program64.exe
IsWoW64Process: FALSE
```