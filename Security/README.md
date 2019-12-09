## Securing Windows Objects

### Windows Kernel Objects Overview

Windows is often described as an "object-based OS" because it exposes much of its underlying functionality to both user mode programs, kernel drivers, and internal system components through objects. In Windows, a kernel object is a single, runtime instance of a statically defined object type. An object type comprises:

- a system-defined data type
- functions that operate on instances of that type
- a set of object attributes

The attributes of an object reflect the object's current state. By design, the internals of kernel objects are opaque. Objects are allocated in system (kernel) memory and are managed by the Windows object manager.

Not all data structures in Windows are represented as kernel objects. Rather, only those data structures that must be shared, protected, named, or made visible to user mode programs are made objects.

**Kernel Object Components**

The common components of kernel objects are :

- Type
- Name
- Directory
- Handle Count
- Pointer Count

The type of an object is itself a pointer to a type object. All of the object types that are exported to user space are visible via the _Winobj.exe_ tool from Sysinternals within the `ObjectTypes` namespace of the Object Manager. 

Note that a kernel object has two distinct reference counts: a handle count and a reference count. An object is only ever destroyed by the object manager when both of these counts reach zero.

### Kernel Object Handles

Because kernel objects reside in system space, the OS must make these objects available to user space via some indirect mechanism. This is accomplished via handles (the `HANDLE` data type). A handle is simply an index into a per-process _handle table_ that logically references a kernel object that resides in system space. This method implies that kernel objects may be easily shared between processes as distinct processes may request handles to the same kernel object. Such parallel requests may be allowed or denied based on the sharing attributes of the object.

A `HANDLE` is really just an opaque type that wraps a `DWORD`. Handle values begin at index 4 and increment by 4. 0 is never a valid handle value, and is often used to test for failed API calls.

**Handle Internals**

A handle acquired by a process points to a small data structure in system space that indirectly references the object in question. This data structure consists of the following components:

- Pointer to the object header for the object
- Access mask
- Flags:
    - Audit on Close: closing the handle will generate an audit message
    - Inheritance: controls inheritance semantics
    - Protect from Close: calls to close the handle will fail

The properties of the underlying handle data structure itself may be queried and updated via the following API:

- `GetHandleInformation()`
- `SetHandleInformation()`

**Pseudo-Handles**

The following functions return a pseudo-handle, rather than a traditional handle to the requested object:

- `GetCurrentProcess()`              -> -1
- `GetCurrentThread()`               -> -2
- `GetCurrentProcessToken()`         -> -4
- `GetCurrentThreadToken()`          -> -5
- `GetCurrentThreadEffectiveToken()` -> -6

### Object Names

Kernel objects are managed by the Object Manager. For objects that may be named, the names are relative to the relevant Object Manager namespace. For instance, if one uses `CreateMutex()` to create a mutex object in a regular, user mode program via local, interactive logon, the resulting object would reside under the `\Sessions\1\BasedNamedObjects` Object Manager namespace (here, the actual session number of 1 is not guaranteed, but is just there to illustrate it is NOT 0).

One may also explicitly specify the fully-qualified name for an object at the time of creation. Thus, if the name of the mutex in the above call to `CreateMutex()` were `"\Global\MyMutex"`, the mutex object would be created in the Session 0 `BaseNamedObjects` Object Manager namespace, allowing the object to be shared across sessions. 

### Sharing Kernel Objects

There are three (3) distinct mechanisms by which kernel objects may be shared among processes:

- Sharing by name
- Sharing by inheritance
- Duplicating handles

Sharing by name is accomplished by simply having cooperating processes that create / open handles to the same kernel object by name.

Duplicating handles is accomplished via the `DuplicateHandle()` API. The function call itself is straightforward, but the complication arises when the process that performs the handle duplication (on behalf of the process that wants the duplicated handle) needs to communicate the fact that it has performed this duplication to the other process. Some other IPC mechanism is required to complete the picture.

### Private Object Namespaces

One of the fundamental ways in which kernel objects may be secured is through limiting their visibility. The above section covers the ways in which kernel objects may be shared, the simplest of which is by accessing a named kernel object by name. However, what if one wishes to create a named kernel object and make it available for sharing only to particular cooperating processes, while hiding it from others. Private object namespaces achieve exactly this.

Creating a private object namespace requires two (2) steps.

First, one creates a _boundary descriptor_ object that describes the entities (by SID) that are able to access the private object namespace. Manage boundary descriptors via the following APIs:

- `CreateBoundaryDescriptor()`
- `AddSIDToBoundaryDescriptor()`
- `AddIntegrityLabelToBoundaryDescriptor()`
- `DeleteBoundaryDescriptor()`

The boundary descriptor is then used in the following calls to create the private namespace:

- `CreatePrivateNamespace()`
- `OpenPrivateNamespace()`
- `ClosePrivateNamespace()`

Notice that both boundary descriptor objects and private namespace objects have their own "delete" functions; this is because both of these objects are not kernel objects, despite the fact that the functions that create them return handles. 

Once the private namespace has been created, other processes may utilize the private namespace alias to access named kernel objects within that namespace.

### Aside: Other Object Types

Windows recognizes objects other than kernel objects, including _user objects_ and _GDI objects_.

User objects are things like:

- Windows (`HWND`)
- Menus (`HMENU`)
- Hooks (`HHOOK`)

Accordingly, GDI objects are objects relating strictly to the graphical device interface API.

### Security Attributes

Nearly all of the Windows API calls that begin with the `Create` prefix allow one to specify the security attributes of the object that is under construction. The security attributes are specified by way of a `SECURITY_ATTRIBUTES` structure.

```
typedef struct _SECURITY_ATTRIBUTES
{
    DWORD nLength,
    LPVOID lpSecurityDescriptor,
    BOOL bInheritHandle
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES;
```

It is clear from the structure definition that the pointer to the security descriptor is the primary member of interest of this structure. The `bInheritHandle` flag specifies whether the handle may be inherited by other processes (e.g. children created by `CreateProcess()`).

### Security Descriptor

Object _security descriptors_ are one side of the object security equation (the other being _access tokens_). Among other things, an object's security descriptor specifies the entities that may performa ations on an objects and which actions they may perform.

An object security descriptor has multiple, distinct access control lists embedded within it:

- _DACL_ -> discretionary access control list
    - Access control entries in this list specify a SID and the rights granted to that SID
- _SACL_ -> system access control list
    - Specifies which actions by which users should be logged
    - If a SACL is null, no auditing occurs for that object 

A new security descriptor object may be initialized with the following APIs:

- `InitializeSecurityDescriptor()`
- `SetSecurityDescriptorOwner()`
- `SetSecurityDescriptorGroup()`
- `InitializeAcl()`
- `AddAccessDeniedAce()`
- `AddAccessAllowedAce()`
- `AddAuditAccessAce()`
- `SetSecurityDescriptorDacl()`
- `SetSecurityDescriptorSacl()`

Notice that there is no concept of a "denied" ACE for system ACLs.

The access control entries of an object's DACL and SACL are scanned sequentially by the security reference monitor when performing an access check. Therefore, the order in which ACEs are added to a security descriptor's DACL and SACL is important.

An ACE consists of a SID and the _access mask_ associated with that SID. The access mask specifies the access rights to be allowed or denied to the specified SID. 

Mask values will vary by object type. So how do we determine the appropriate ACE mask values to apply to a given kernel object?

- Read the docs (sometimes useful)
- Read header files _winnt.h_ and _winbase.h_

**Security Descriptor Control Flags**

The control flags control the meaning assigned to the security descriptor.

- `GetSecurityDescriptorControl()`
- `SetSecurityDescriptorControl()`

In addition to the latter of the two functions above, the control bits of a security descriptor may also be modified implicitly by other security descriptor manipulation APIs.

The control flags for a security descriptor specify properties such as the inheritance properties of the security descriptor DACL and SACL, and whether or not SD is absolute or self-relative.

**Absolute vs Self-Relative Security Descriptors**

An absolute security descriptor is the kind which is generated by a call to `InitializeSecurityDescriptor()` - these are the security descriptors that are often manipulated by processes. However, absolute security descriptors cannot be associated with persistent objects like files because they reference data in memory and will thus become invalidated when the process manipulated the security descriptor terminates.

Windows defines a second type of security descriptor, a self-relative security descriptor, that is a compacted version of the absolute security descriptor. Windows automatically consolidates an absolute SD to a self-relative SD when the SD is associated with a persistent object.

Convert between the two types of SDs manually using the following APIs:

- `MakeAbsoluteSD()`
- `MakeSelfRelativeSD()`

**Security Descriptor APIs**

Query security descriptors with the following API functions:

- `GetSecurityInfo()`
- `GetKernelObjectSecurity()`
- `SetKernelObjectSecurity()`
- `GetFileSecurity()`
- `GetNamedSecurityInfo()`
- `GetSecurityDescriptorOwner()`
- `GetSecurityDescriptorGroup()`
- `GetSecurityDescriptorDacl()`
- `GetSecurityDescriptorSacl()`
- `GetAclInformation()`
- `GetAce()`

Further APIs exist specifically for querying the security descriptor of files:

- `GetFileSecurity()`
- `SetFileSecurity()`

**Kernel Debugging**

- The pointer to the security descriptor is in the object header of the `EPROCESS` object
- Use the `!sd` command to dump the security descriptor
- Caveat here about bitwise-anding the pointer first because some bits of the address are used as flags??? (&-10 on 64 bit system) e.g !sd <addr> & -10 (64 bit)

### Security Identifier (SID)

Instead of using names to identify entities which perform actions within a system, Windows uses SIDs. 

**Which entities are issued a SID?**

- Users
- Local and domain groups
- Local computers
- Domains
- Domain members
- Services

**SID components**

- ‘S’ prefix
- SID structure revision number
- 48 bit authority value -> who issued the SID
- Variable number of
    - 32 bit sub authority values
    - Relative identifier (RID) values

Standard Windows APIs allow one to query the SID for a specified account name and vice versa.

### Access Tokens (aka Tokens)

**What is a token?**

An _access token_ is associated with a process. The token specifies the owning user of the process and the groups to which the user belongs.

- Identify the security context of a process or thread
- Identify a user’s (entity’s?) credentials, and thus their privileges
- When user logs in, LSASS checks if the user is a member of a powerful group or possesses a powerful privilege; if this is the case, LSASS creates a restricted token (a filtered admin token) for the user in addition to the powerful one, and creates a logon session for each

**Access Tokens APIs**

- `LogonUser()`
- `CreateProcessAsUser()`
- `CreateProcessWithLogon()`
- `AdjustTokenGroups()`
- `AdjustTokenPrivileges()`
- `OpenProcessToken()`
- `CreateProcessWithToken()`
- `DuplicateToken()`
- `DuplicateTokenEx()`

**Token Components**

- Group SIDs -> identify the groups to which the user represented by the token belongs
- Privilege array
- Default primary group, default DACL -> used when the process associated with this token creates an object, applied as default security policy
- Many other "informational" fields

**Trust ACEs**

- Allow protected processes and PPL to make objects accessible only to other protected processes
- Identified by well-known SIDs
- Higher trust SID = more powerful token
- Listed in the kernel debugger as "TrustLevelSid" within the token object

**Default Security Descriptor**

One other member of an access token is a default security descriptor. This is the security descriptor that will be applied to all objects created by the process to which the token is attached in the event that a security descriptor is not explicitly specified (passing `NULL` or `nullptr` during object creation).

**Kernel Debugging**

- Use dt _TOKEN to view token structure
- Use the !token command to examine token for a process

### Integrity Levels

- Orthogonal to access control?
- We can isolate code and data running within the context of a user account
- 6 levels commonly used
    - 0 = untrusted
    - 5 = protected
- For instance, mark an untrusted process like a web browser with a low integrity level, will prevent it from doing certain things
- Processes have integrity levels, but so do other objects 
- Windows enforces a "No Write Up" policy by default such that a process may not "write" any object that is configured with a higher integrity level
    - e.g. Run notepad.exe at low intergity level and you won't be able to save a new file to any location other than the LocalLow directory
- Implicit medium integrity level system-wide

### References

- _Windows System Programming, 4th Edition_ Pages 519-544
- _Windows Internals, 7th Edition_ Pages 619-666
- _Windows Internals, 7th Edition_ Pages 668-675
- _Windows 10 System Programming_ Chapter 3