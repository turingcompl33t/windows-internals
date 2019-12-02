## Trustlets (Secure Processes)

The Secure Kernel provides fewer than 50 system calls that are accessible to Trustlets running in IUM. The purpose of this feature is to minimize the attack surface and exposure of the Secure Kernel. For this reason, many common operations (e.g. file IO, registry operations) are impossible in the context of a trustlet. Microsoft intends trustlets to serve as the "isolated work-horse backends" to more fully-featured front-end applications running at VTL 0. Trustlets communicate with their VTL 0 "agents" via ALPC.

### Trustlet Properties

- Regular portable executable files
- May only import a limited subset of system DLLs because they are limited in the system calls that they may perform
- May import from an IUM-specific DLL, `Iumbase`, which implements the base IUM system API
- Contain a special section in the binary, _.tPolicy_, that defines metadata that allows the Secure Kernel to enforce policy settings regarding VTL 0 access to the trustlet
- Signed with a special certificate that identifies it as a trustlet 

Policy options that may be specified for a trustlet include:

- Enable / disable ETW tracing
- Debugging configuration
- Enable / disable crash dumps
- Public key used to encrypt crash dumps
- GUID used to identify crash dumps
- Enabling "powerful" VTL 1 capabilities e.g. DMA, Create Secure Section API, etc.

### System Built-in Trustlets

Windows currently comes configured with the following five (5) trustlets:

- `lsalso.exe` (1): the credential and key guard trustlet
- `vmsp.exe` (2): the vTPM trustlet
- `Unknown` (3): the vTPM key enrollment trustlet
- `Biolso.exe` (4): the secure biometrics trustlet
- `Fslso.exe` (5): the secure frame server trustlet

These trustlets differ in the policy options that they come configured with.

### Launching a Trustlet

Is this possible??

### References

- _Windows Internals, 7th Edition Part 1_ Pages 123-129
- [Battle of the SKM and IUM: How Windows 10 Rewrites OS Architecture (Video)](https://www.youtube.com/watch?v=LqaWIn4y26E&t=335s)
- [Battle of the SKM and IUM: How Windows 10 Rewrites OS Architecture (Slides)](http://www.alex-ionescu.com/blackhat2015.pdf)