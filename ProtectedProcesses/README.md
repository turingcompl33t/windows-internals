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

- Protected in the same sense as protected processes
- Different signers have different trust levels, so some PPL are more protected than others

**Hierarchy**

Protected processes always trump PPL, then higher-value signer PPL have access to lower ones, but not vice versa.

## Programming

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

## References

- Windows Internals 7th Edition Part 1 (Pages 113-120)