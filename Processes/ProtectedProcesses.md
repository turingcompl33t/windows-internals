### Protected Processes

**Origins**

- Protected processes originally arose from the need to enforce additional security for the purposes of Digital Rights Management (DRM)
- Introduced in Vista / Server 2008
- Like regular processes, but with additional constraints on the access rights that other processes (even those with Administrator privileges) may request
- Any application may request the creation of a protected process, but the OS will only grant this if the image file associated with the process is digitally signed by a Windows Media Certificate

**Examples**

- The System process runs as a protected process
- Why?
    - The System process handle table contains all kernel handles on the system
    - Kernel drivers may map user-mode memory in the System process

**Access Rights**

The following access rights are the only ones that may be granted for protected processes

- PROCESS_QUERY_LIMITED_INFORMATION
- PROCESS_SET_LIMITED_INFORMATION
- PROCESS_TERMINATE
- PROCESS_SUSPEND_RESUME

### Protected Process Light (PPL)

- An extension the the protected process model
- Introduced in 8.1 / Server 2012 R2

**PPL vs Protected Processes**

**Origins**

- Protected in the same sense as protected processes
- Different signers have different trust levels, so some PPL are more protected than others

**Hierarchy**

Protected processes always trump PPL, then higher-value signer PPL have access to lower ones, but not vice versa.

### Programming

The protection level of a process impacts the DLLs it will be allowed to load. This was done explicitly to prevent things like, for example, DLL injections on protected processes that then allow the malicious injected code to run with the authority of the protected process.

**Valid Process Protection Levels**

Legal values for the protection flag stored in the EPROCESS structure. Protections are listed by signer, from high to low power.

| Internal Protection Process Level Symbol | Protection Type | Signer       |
|------------------------------------------|-----------------|--------------|
| PS_PROTECTED_SYSTEM (0x72)               | Protected       | WinSystem    |
| PS_PROTECTED_WINTCB (0x62)               | Protected       | WinTcb       |
| PS_PROTECTED_WINTCB_LIGHT (0x61)         | Protected Light | WinTcb       |
| PS_PROTECTED_WINDOWS (0x52)              | Protected       | Windows      |
| PS_PROTECTED_WINDOWS_LIGHT (0x51)        | Protected Light | Windows      |
| PS_PROTECTED_LSA_LIGHT (0x41)            | Protected Light | Lsa          |
| PS_PROTECTED_ANTIMALWARE_LIGHT (0x31)    | Protected Light | Anti-Malware |
| PS_PROTECTED_AUTHENTICODE (0x21)         | Protected       | Authenticode |
| PS_PROTECTED_AUTHENTICODE_LIGHT (0x11)   | Protected Light | Authenticode |
| PS_PROTECTED_NONE (0x0)                  | None            | None         |

**Signers and Levels**

| Signer Name                   | Level | Used For                            |
|-------------------------------|-------|-------------------------------------|
| PsProtectedSignerWinSystem    | 7     | System and minimal processes        |
| PsProtectedSignerWinTcb       | 6     | Critical Windows components         |
| PsProtectedSignerWindows      | 5     | Components handling sensitive data  |
| PsProtectedSignerLsa          | 4     | Lsass.exe (if running protected)    |
| PsProtectedSignerAntimalware  | 3     | Anti-malware services and processes |
| PsProtectedSignerCodeGen      | 2     | .NET native code generation         |
| PsProtectedSignerAuthenticode | 1     | Hosting DRM, loading usermode fonts |
| PsProtectedSignerNone         | 0     | Not valid                           |

**Query Protected Service Information**

```
sc qprotection <SERVICE NAME>
```

### Basic Exploration

For starters, what are we even looking for? The `Protection` field in the EPROCESS structure:

```
kd> dt nt!_EPROCESS Protection
   +0x6fa Protection : _PS_PROTECTION
kd> dt nt!_PS_PROTECTION
   +0x000 Level            : UChar
   +0x000 Type             : Pos 0, 3 Bits
   +0x000 Audit            : Pos 3, 1 Bit
   +0x000 Signer           : Pos 4, 4 Bits
```

And how do we interpret the `Type` and `Signer` subfields of this structure?

```
kd> dt nt!_PS_PROTECTED_TYPE
   PsProtectedTypeNone = 0n0
   PsProtectedTypeProtectedLight = 0n1
   PsProtectedTypeProtected = 0n2
   PsProtectedTypeMax = 0n3
kd> dt nt!_PS_PROTECTED_SIGNER
   PsProtectedSignerNone = 0n0
   PsProtectedSignerAuthenticode = 0n1
   PsProtectedSignerCodeGen = 0n2
   PsProtectedSignerAntimalware = 0n3
   PsProtectedSignerLsa = 0n4
   PsProtectedSignerWindows = 0n5
   PsProtectedSignerWinTcb = 0n6
   PsProtectedSignerWinSystem = 0n7
   PsProtectedSignerApp = 0n8
   PsProtectedSignerMax = 0n9
```

We now have all the information we need to take a look at the protection status of the processes on the debuggee. First we have a look at `lsass.exe`.

```
kd> !process 0 0 lsass.exe
PROCESS ffff9e02f4e11080
kd> ?? ((nt!_EPROCESS*)0xffff9e02f4e11080)->Protection.Level
unsigned char 0x00 ''
```

So `lsass.exe` is not running as a protected process on this system; this is to be expected on modern system where `lsass.exe` is protected by VBS and thus need not rely on protected process or PPL protections. 

Surely the System process is protected... 

```
kd> !process 0 0 System
PROCESS ffff9e02f0480040
kd> ?? ((nt!_EPROCESS*)0xffff9e02f0480040)->Protection.Level
unsigned char 0x72 'r
```

So we get the `Signer` for the System process as `PsProtectedSignerWinSystem` and the `Type` as `PsProtectedTypeProtected`.

Finally, lets see if we can find a PPL.

```
kd> !process 0 0 MsMpEng.exe
PROCESS ffff9e02f6a04280
kd> ?? ((nt!_EPROCESS*)0xffff9e02f6a04280)->Protection.Level
unsigned char 0x31 '1'
```

For Windows Defender we have the `Signer` as `PsProtectedSignerAntimalware` and the `Type` as `PsProtectedTypeProtectedLight`. Precisely as expected.

### Dump All Protected Processes

Windbg script, shamelessly ripped off from Alex Ionescu's [post](http://www.alex-ionescu.com/?p=116).

```
!for_each_process "r? @$t0 = (nt!_EPROCESS*) @#Process; .if @@(@$t0->Protection.Level) {.printf /D \"%08x <b>[%85msu]</b> level: <b>%02x</b>\\n\", @@(@$t0->UniqueProcessId), @@(&@$t0->SeAuditProcessCreationInfo.ImageFileName->Name), @@(@$t0->Protection.Level)}"
```

Sample output:

```
kd> vertarget
Windows 10 Kernel Version 18362 MP (1 procs) Free x64
Product: WinNt, suite: TerminalServer SingleUserTS Personal
Built by: 18362.1.amd64fre.19h1_release.190318-1202
kd> !for_each_process "...<script omitted>..."
00000004 [                                                                                     ] level: 72
00000044 [                                                                             Registry] level: 72
00000178 [                                    \Device\HarddiskVolume2\Windows\System32\smss.exe] level: 61
000001cc [                                   \Device\HarddiskVolume2\Windows\System32\csrss.exe] level: 61
00000210 [                                 \Device\HarddiskVolume2\Windows\System32\wininit.exe] level: 61
00000218 [                                   \Device\HarddiskVolume2\Windows\System32\csrss.exe] level: 61
0000028c [                                \Device\HarddiskVolume2\Windows\System32\services.exe] level: 61
000001a0 [                                 \Device\HarddiskVolume2\Windows\System32\svchost.exe] level: 51
000004a0 [                                    \Device\HarddiskVolume2\Windows\System32\upfc.exe] level: 51
000005dc [                                                                       MemCompression] level: 72
0000089c [                   \Device\HarddiskVolume2\Program Files\Windows Defender\MsMpEng.exe] level: 31
00000a68 [                    \Device\HarddiskVolume2\Program Files\Windows Defender\NisSrv.exe] level: 31
00000ed0 [    \Device\HarddiskVolume2\Windows\Microsoft.NET\Framework64\v4.0.30319\mscorsvw.exe] level: 21
00000f20 [                                  \Device\HarddiskVolume2\Windows\System32\sppsvc.exe] level: 52
```

### References

- Windows Internals 7th Edition Part 1 (Pages 113-120)
- [The Evolution of Protected Processes Part 1](http://www.alex-ionescu.com/?p=97)
- [The Evolution of Protected Processes Part 2](http://www.alex-ionescu.com/?p=116)
- [The Evolution of Protected Processes Part 3](http://www.alex-ionescu.com/?p=146)
- [Injecting Code into Windows Protected Processes using COM Part 1](https://googleprojectzero.blogspot.com/2018/10/injecting-code-into-windows-protected.html)
- [Injecting Code into Windows Protected Processes using COM Part 2](https://googleprojectzero.blogspot.com/2018/11/injecting-code-into-windows-protected.html)
- [Unknown Kown DLLs and other Code Integrity Trust Violations](http://www.alex-ionescu.com/Publications/Recon/recon2018.pdf)