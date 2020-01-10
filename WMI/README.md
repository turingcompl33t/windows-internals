## Windows Management Instrumentation (WMI)

WMI is an implenentation of Web-Based Enterprise Management (WBEM) - a standard defined by the Distributed Management Task Force (DMTF). At its core, WBEM describes the design of an enterprise data collection and management facility with the ability to manage both local and remote systems in a uniform manner.

### WMI Architecture

![WMI Architecture](Local/WmiArchitecture)

WMI consists of four (4) primary components:

- Management Applications
- WMI Infrastructure
- Providers
- Managed Objects

**Management Applications**

"Management applications are Windows applications that access and display or process data related to managed objects."

Examples of management applications include:
- Simple performance monitoring utilities that utilize WMI provider information rather than OS performance data APIs in order to obtain performance information
- More complex enterprise-level tools that allow administrators to both get and set properties of enterprise components (in the form of managed objects)

Management applications may make use of several available WMI APIs including:
- COM API
- Scripting API
- Others

The COM API is the primary (most powerful) API available to WMI management applications. WMI management applications act as COM clients.

**WMI Infrastructure**

The WMI infrastructure is the layer that ties together WMI management applications and WMI providers. That is, WMI management application do not communicate directly with WMI providers; instead, all WMI operations issued by a management application are dispatched to the WMI infrastructure which then handles the request accordingly.

The heart of WMI infrastructure is the Common Information Model (CIM) Object Manager (CIMOM). 

**Providers**

WMI providers typically interact with the WMI infrastructure via the COM API. WMI providers act as COM servers.

Well-known Windows WMI providers include:
- Performance API
- Windows Registry
- Event Manager
- Active Directory
- Select Device Drivers

A provider may implement one or more features, allowing it provide one or more of the following:
- Class
- Instance
- Property
- Method
- Event
- Event Consumer

**Managed Objects**

Managed objects are those resources (physical or logical) whose properties are exposed to the WMI infrastructure by providers.

### Common Information Model (CIM), CIM Object Manager (CIMOM), Managed Object Format (MOF)

The CIM is a specification that describes how management systems represent some device or resource. The CIM specification allows developers to design representations as classes, and this allows developers to take advantage of many of the concepts from object-oriented programming such as inheritance and composition. 

CIM class categories include:
- Core Model: e.g CIM_ManagedSystemElement, CIM_LogicalElement, CIM_PhysicalElement
- Common Model: e.g. CIM_FileSystem
- Extended Model: e.g. Win32_PageFile, Win32ShortcutFile, Win32_NtEvenlLogFile

The MOF language is the language utilized by developers to implement a CIM representation for some resource. The MOF used to implement a CIM class may be queried via the _WbemTest_ tool that ships with Windows.

**Static vs Dynamic Classes**

Static classes in WMI are those for which data is stored in the WMI repository, implying that the WMI infrastucture consults the repository when a management application queries for information relating to such a class.

In contrast, when a management application queries for information regarding a dynamic class, the WMI infrastructure must make a request to a provider for the specified information.

### WMI Namespaces

WMI utilizes namespaces to organize the WMI system hierarchically. The root namespace is named `root`. There are four (4) pre-defined subnamespaces of the root namespace:
- `CIMV2`
- `Default`
- `Security`
- `WMI`

Windows defines a number of other subnamespaces that reside directly beneath the root namespace. Unlike a directory hierarchy in a filesystem, a WMI namespaces are only ever one level deep.

### WMI Implementation

The WMI service runs in a shared `svchost` process. This service loads providers into the _Wmiprvse.exe_ provider hosting process, which is launched as a child of the RPC service process.

Most WMI resources on a system reside in the `%SystemRoot%\System32` and `%SystemRoot%\System32\Wbem` directories. This includes resources such as MOF files and provider DLLs.

The WMI repository (identical to the CIMOM repository) is implemented as a database file stored under `%SystemRoot%\System32\Wbem\Repository`. 

WMI implements security at the namespace level via the standard Windows security model mechanisms (i.e. objects and security descriptors)

### References

- _Windows Internals, 6th Edition Part 1_ Pages 342-353