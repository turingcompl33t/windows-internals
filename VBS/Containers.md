## Windows Container Technology

Hooray!

### Windows Container Classes

Helium (_He_)
- Lightest weight container
- Application isolated via registry and filesystem virtualization
- Used for Centennial as a bridge
- No security gurantees

Argon (_Ar_)
- Container providing and isolated user session
- Shares kernel
- Used to increase density in server and cloud environments
- Not a security boundary

Krypton (_Kr)
- Container utilizing a lightweight VM
- Utilizes a separate kernel from the host -> resistant to kernel attacks

Xeon (_Xe_)
- Container that uses a lightweight VM
- Hypervisory boundary
- Used in hostile, multi-tenant hosting
- Commercially known as "Hyper-V Container"

### Krypton Container Technology

Krypton container technology is the container technology selected by MS to implement Windows Defender Application Guard.

Features:
- Direct map -> sharing of resources between guest and host
- Memory enlightenment -> VM memory mapped by virtual memory rather than physical memory
- Integrated scheduler -> to the host kernel, the VM looks just like another process

### References

- [Securing Windows Defender Application Guard (Video)](https://www.youtube.com/watch?v=X0QaP9xR7sc)