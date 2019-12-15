## Windows Hyper-V Hypervisor

### Partitions

Partitions are a critical concept in Windows hypervisor technology. A partition is essentially just an operating system instance running on top of the hypervisor. A system must have a single _parent_ (or _root_) partition and may have many _child_ partitions.

One of the objectives of the Windows hypervisor model is to make the hypervisor itself a relatively small and simple piece of software. To accomplish this, most of the work of managing the hypervisor and the other partitions running on top of it is performed by the parent partition. Thus, it is the parent partition on the system that actually runs the _hypervisor stack_ - the collection of components that allow the hypervisor to communicate with regular Windows device drivers.

The parent partition runs a single instance of the _virtual machine manager service_ which is reponsible for implementing the WMI interface to the hypervisor, as well as controlling configuration settings related to all child partitions.

For each child partition, the parent partition runs an instance of the _virtual machine worker process_ which performs work in the parent partition on behalf of child partitions.

The parent partition hosts _virtualization service providers_ (VSP) while child partitions host _virtualization service clients (VSC).

### Hypervisor Interface

Partitions communicate with the hypervisor by way of a kernel device driver called _Winhv.sys_. This driver functions as a library that exports the hypervisor interface through _hypercalls_. 

At boot time, the _hvboot.sys_ driver registers the hypervisor software with the hardware itself to configure the system for virtualization. 

### Enlightenments

Windows uses the term _enlightenments_ to refer to kernel code modifications in which a partition:

- Knows or determines that it is running on top of a hypervisor rather than having direct access to physical hardware
- Uses this determination to optimize certain operations

One particular example of an enlightenment concerns the TLB. On a process context switch, the OS typically executes an instruction to flush the TLB. On an enlightened partiton, however, the virtualized OS will instead issue a hypercall to request that the hypervisor flush only the portions of the TLB that contain translations related to this particular partition.