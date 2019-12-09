## Windows Object Management

Windows is often referred to as an _object-based_ operating system because it implements an _object model_ to provide consistent and secure access to the internal services implemented by the Windows executive. The _Object Manager_ is the executive component that implements object management on Windows systems. Some of the goals of the Object Manager and the Windows object model include:

- Provide a common, uniform mechanism for accessing system resources
- Isolate object protection in a single OS location
- Provide a mechanism to "charge" processes for system resource usage
- Establish a globally-valid object naming scheme

### Object Types

Windows recognizes three (3) distinct types of objects internally:

- Executive Objects
- Kernel Objects
- User / GDI Objects

Executive objects are typically composed of one or more "primitive" kernel objects. These kernel objects are only utilized internally by the Executive, while executive objects are exported for use throughout the system.

Do not confuse terminology here: often the term "kernel object" is used to refer to executive objects. The precise name used to identify these object is immaterial so long as one keeps in mind that their are two "levels" of objects recognized internally by the Windows kernel: those that it exports for system-wide object management, and those that it does not.

### Common Executive Objects

The executive implements many object types; some common object types include:

- Process
- Thread
- Job
- Section
- File
- Token
- Event
- Semaphore
- Mutex
- Timer
- Key
- WindowStation
- Desktop

**Aside: Mutex vs Mutant**

Why is the Windows mutual exclusion primitive referred to as a "mutant" internally?

Windows originally supported multiple subsystems, including the OS/2 subsystem. Because Windows had to support this subsystem, it required a synchronization primitive that was compatible with it, and called this primitive a "mutex". However, the OS/2 mutual exclusion semantics were less-than perfect, so Windows defined its own mutual exclusion primitive and referred to it internally as a "mutant". Now, with the disappearance of OS/2 support, the user-space mutual exclusion primitive (with more desirable semantics) is now referred to as a "mutex", but the underlying implementation remains the "mutant" type for these historical reasons.

### Object Structure

At a high level, an object consists of two (2) components:

- Object Header
- Object Body

The object header consists of information that is common to every object type, while the object body is specific to the particular object type.

In addition to these two components, the object manager also recognizes five (5) optional subheaders whose presence or absence is reflected by a bitfield in the object header.

**Type Objects**

In a somewhat recursive manner, Windows defines object types in terms of objects. That is, the object header for an executive object contains a _Type Index_ field that represents the index into an object type table which in turn contains pointers to type objects that describe object types.

**Object Methods**

The type object for a particular object type may define _object methods_ which are essentially a generalization of C++ constructors / destructors. That is, object methods define operations that the Object Manager will perform at certain points in the lifetime of the object, including creation, destruction, handle opening, etc.

Most object methods are generic and deal only with the information provided by the object header. However, certain object types may define their own methods that will then be invoked by the Object Manager in response to a particular object lifecycle event.

For instance, the IO System registers an object method for file objects that is invoked on file close operations. This method checks the file object to determine if the process that is closing its handle to the file has any outstanding locks on the file, and, if it does, releases such locks. This situation illustrates a case in which a custom object method is necessary because the Object Manager "knows nothing" about file locks.

Another important example is the parse method. The parse method may be specified in order to allow the object manager to relinquish control of name resolution to an alternate entity, which is particularly useful in the case of alternate namespaces. In addition to the Object Manager namespace, Windows also recognizes the registry namespace (implemented by the Configuration Manger) and the filesystem namespace (implemented by the IO Manager). Thus, for operations involving objects implemented by these executive subsystems, the Object Manager may invoke a custom parse method to allow either of these executive components to finish the name resolution process.

### Object Handles and Process Handle Table

Object handles provide indirect pointers to executive objects. Handle are convenient for a variety of reasons, two of the most important being that referring to an object by its handle rather than by name is faster and the handle interface is consistent across all objects regardless of object type. Perhaps the most important aspect of handles, at least as they relate to achieving the design goals of the Object Manager, is the fact that the layer of indirection allows the Object Manager to interdict on every resource acquisition operation performed by applications and take appropriate actions (e.g. deny the request, update a reference count, decrement a quota value, etc.).

Note here that while kernel components (e.g. kernel drivers) may refer to executive objects directly via pointers, it is often more tedious to do so, and in some circumstances prevented altogether by the system by maintaining types that are opaque even to kernel components. For this reason, it is not uncommon to see even kernel drivers utilize the same APIs (native API functions) that user applications end up using to acquire handles to executive objects.

The process handle table is located via a field in the process `EPROCESS` structure. While one may think about the process handle table as a simple linear array of handles, it is actually a three-level scheme similar to the virtual address translation scheme on x86 platforms. The use of the three-level scheme allows for a theoretical limit of over 16 million handles per process.

The system saves memory by only allocating the lowest-level handle table at the time of process creation; other levels are added on-demand. The tables are allocated at page granularity; page size on Windows systems is 4K, and the size of a handle table entry is 8 bytes (on x86, on x64 systems it is 12 bytes), allowing for a total of 511 handles per page, with the final "slot" leftover for "handle auditing." Note that handle tables are allocated from paged pool, thus it is likely that paged pool will be exhausted prior to opening the maximum number of handles available to a process.

The kernel implements its own handle table for internal use. The kernel handle table is accessible only when running in kernel mode and from any process context. Kernel handles are distinguished by the fact that the high bit of the handle is set (that is, the handle table index that the handle represents is greater than 0x80000000).

### Aside: Reserve Objects

Reserve objects are essentially a mechanism by which an application may pre-allocate memory for future object allocation. This allows the application to ensure that object allocation will always succeed, even in low-memory situations, and thus the functionality that must be performed under these conditions will not be inhibited.

Currently, reserve object use is reserved (pun intended) for system components.

### Object Security

Object security is covered in detail in another section of this repository. All I will say about it here is the following: there is a division of labor between the Object Manager and the Security Reference Monitor (SRM) when it comes to dealing with object security:

- When an application requests a new handle to an object, the Object Manager invokes the SRM to determine if the access is allowed; if it is, the SRM returns the access rights that the application has been granted for that object, and the Object Manager stores these in the corresponding handle
- Now, whenever the application makes use of this handle to manipulate or query the object, the Object Manager simply compares the requested operation with the access mask for the object's handle to determine if the operation is permitted; there is no need to consult the SRM on individual operations once the handle has been acquired

### References

- _Windows Internals, 6th Edition Part 1_ Pages 140-176