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