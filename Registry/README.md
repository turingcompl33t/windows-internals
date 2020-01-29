## Windows Registry

The Windows registry is utilized at both the system and application level to manage configuration data. The registry is managed by the Configuration Manager executive component.

### Registry Data Types

The recognizes the following twelve (12) data types.

- `REG_BINARY`: binary data in any form
- `REG_DWORD`: 32-bit number
- `REG_QWORD`: 64-bit number
- `REG_DWORD_LITTLE_ENDIAN`: a 32-bit number in little-endian format, the default storage format, thus equivalent to REQ_DWORD
- `REG_QWORD_LITTLE_ENDIAN`: a 64-bit number in little-endian-format, the default storage format, thus equivalent to the REG_QWORD
- `REG_DWORD_BIG_ENDIAN`: a 32-bit number in big-endian format
- `REG_EXPAND_SZ: null terminated string with unexpanded references to environment variables e.g. `%PATH%`
    - May be unicode or ANSI string depending on the functions used to manipulate 
- `REG_LINK`: unicode symbolic link
- `REG_MULTI_SZ`: array of null-terminated strings, each of which is terminated by two null characters
- `REG_NONE`: no defined value type
- `REG_RESOURCE_LIST`: device driver resource list
- `REG_SZ`: null-terminated string
    - May be unicode or ANSI depending on the functions used to manipulate

### Registry Architecture

The Windows registry is a hierarchical database of configuration data that acts as the repository for both system and user-specific settings. The primary source of data that composes the Windows registry is a set of database files stored on disk called registry _hives_. Registry hives are implemented in a binary format specified by Microsoft. In addition to the data stored in registry hives, the Windows registry also provides access to some dynamic information that is maintained in-memory by the system. The primary example of this is the system performance data that the registry makes available.

The registry is a hierarchical structure that is organized in a manner that is analogous to a filesystem. The registry is composed of _keys_ and _values_. Keys are analogous to filesystem directories and may contain other keys (called _subkeys_) or values. A registry value is analogous to a filesystem file - it stores data as well as other metadata such as a name that is used to identify the value. 

### Logical Structure

The registry contains six (6) root keys; root keys may not be added or removed.

**`HKEY_CURRENT_USER`**

This root key is actually a link to the subkey for the current user profile under the `HKEY_USERS` root key.

This key contains data regarding the preferences and software configuration for the current, locally logged-on user.

**`HKEY_USERS`**

This root key is a true (non-link) root key.

This key contains a subkey for each loaded user profile, as well as one for the "Default" user that is used to store configuration data for programs and services running under system auspices. Note that only loaded profiles are present under this subkey, and may be loaded and unloaded dynamically as needed.

**`HKEY_CLASSES_ROOT`**

This root key is actually a merged view of two other true root keys:

- `HKLM\SOFTWARE\Classes`
- `HKEY_USERS\<SID>\SOFTWARE\Classes`

This key contains three things:

- File extension associations
- COM class registrations
- The virtualized registry root for UAC

**`HKEY_LOCAL_MACHINE`**

This root key is a true (non-link) key.

Important subkeys within the `HKLM` root key include:

- `HKLM\BCD00000000`: the boot configuration database
- `HKLM\HARDWARE`: some hardware to device driver mappings, less important on modern systems
- `HKLM\SAM`: local account and group information, only accessible by Local System
- `HKLM\SECURITY`: systemwide security and user-rights assignments, only accessible by Local System
- `HKLM\SOFTWARE`: systemwide configuration information not needed to boot the system, third party application configurations
- `HKLM\SYSTEM`: systemwide configuration needed to boot the system

**`HKEY_CURRENT_CONFIG`**

This root key is actually a link to the the current hardware profile under `HKLM\SYSTEM\CurrentControlSet\Hardware Profiles\Current`. 

**`HKEY_PERFORMANCE_DATA`**

This root key is a true (non-link) key.

This key is what exposes performance counters on Windows systems. The system does not actually persist this performance data, but rather uses the registry as the means to access dynamic performance data. 

This key is not visible via GUI applications like `regedit.exe` and must be queried programmatically e.g. via the Win32 registry API.

The other available method for accessing performance data is via the Performance Data Helper (PDH) functions available in _pdh.dll_.

### Registry Usage

The Windows registry stores configuration information that controls the behavior of both system and user-defined components (i.e. applications). Ostensibly, the registry must be read from and written to in order to make all of this configuration data relevant to the operation of the system. The primary instances in which registry data is read are the following:
- Early in the system boot process; the system's boot configuration database (BCD) is stored in a registry hive file, so the boot manager must consult the data contained in this hive to determine the appropriate configuration under which to boot
- Later in the boot process; the registry is consulted for assorted configuration data during system initialization such as the list of boot-start drivers
- During the kernel initialization process; the kernel reads registry information during its own internal initialization to determine which drivers should be loaded (system-start drivers) as well as how Executive subsystems like the Memory Manager and Process Manager should be configured
- During user logon; the logon process utilizes registry information to determine the per-user settings that should be applied; this includes important settings such as auto-launch programs and the default user shell 
- During application startup; applications that require configuration data to persist across distinct program instances utilize the registry to persist this information; at startup such applications read from the registry and apply the configuration settings found there

Likewise, common instances of registry write operations include the following:
- Initial application setup; during their initial setup process, applications that require persistent configuration write some default configuration profile to the registy
- Device driver installation; at its most fundamental level, driver installation is merely adding a couple registry entries that make the system aware of the driver and its configuration information
- Application setting change; when the settings for a user-mode application are changed and the application requires that this update be persisted it utilizes the registry to accomplish this

In addition to these common registry use cases, Windows also exposes a robust registry management API that allows developers to read and write the registry on-demand. The `regedit` tool makes use this API and allows arbitrary users (although typically elevation is required for modification) to interact with the registry.

### WoW64 Registry Redirection

Windows on Windows 64 (WoW64) is the technology that allows programs built to target a 32-bit build of Windows to run transparently (without any additional user input) on 64-bit builds of Windows. This technology involves a number of sub-components, one of which is registry redirection support. Under WoW64 registry redirection, WoW64 implements two logical views of the registry: the view seen by native 64-bit processes (the entire registry) and the view seen by WoW64 processes. The system implements these two "views" by splitting the registry at two critical locations within the registry hierarchy:
- `HKLM\SOFTWARE`
- `HKEY_CLASSES_ROOT`

Under these keys, the system creates a registry key with the name `WoW6432Node`. Under this key is stored the 32-bit view of the registry that corresponds to the parent key under which it is mounted. With this infrastructure in place, the WoW64 engine loaded in the address space of the WoW64 process is able to intercept all API calls that manipulate the registry and redirect them to the WoW64 node that corresponds to the originally targeted node.

The Windows registry API allows one to bypass WoW64 redirection in various API calls by manually specifying the version (32-bit or 64-bit) of the key to be queried:
- `KEY_WOW64_64KEY`: explicitly opens the 64-bit version of the key (the view exposed to native 64-bit applications)
- `KEY_WOW64_32KEY`: explicitly opens the 32-bit version of the key (the view exposed to WoW64 applications)

### Windows Registry API

The Windows registry API gives application developers the ability to programmatically interact with the Windows registry. Like most system resources, registry resources are represented in the context of a user-mode process by handles (`HANDLE`s) to registry keys. Therefore, the first step in interacting with the registry via the registry API is to obtain a handle to an open registry key. One may create a new registry key and simultaneously acquire a handle to it via the `RegCreateKeyEx()` function (note that MSDN marks `RegCreateKey()` as deprecated). If instead one wants to acquire a handle to an existing registry key, this may be accomplished via the `RegOpenKeyEx()` function (note that MSDN marks `RegOpenKey()` as deprecated). Both of these functions accept a formal parameter that defines the requested access rights to the key object that the returned handle will possess. Common values for this access mask include:
- `KEY_ALL_ACCESS`
- `KEY_WRITE`
- `KEY_READ`
- `KEY_ENUMERATE_SUBKEYS`

Once a handle to an open registry key has been obtained, one may query the subkeys and values stored underneath the key:
- `RegEnumKeyEx()`: enumerates the subkeys of the specified registry key; note that MSDN marks `RegEnumKey()` as deprecated
- `RegEnumValueEx()`: enumerates the values of the specified registry key

Similarly, one may get and set the values underneath the open registry key with the following functions:
- `RegQueryValueEx()`: retrieves the data and type for the specified value associated with the specified registry key; note that MSDN marks `RegQueryValue()` as deprecated
- `RegSetValueEx()`: sets the data and type for a value associated with the specified registry key; note that MSDN marks `RegSetValue()` as deprecated

The registry API exposes the `RegDeleteKey()`, `RegDeleteKeyEx()`, and `RegDeleteValue()` functions to delete registry keys and values associated with a specified open registry key, respectively.

Finally, once registry manipulations are complete, the registry API exposes the `RegCloseKey()` function to release the handle to the open registry key.

As mentioned in the previous item describing WoW64 and registry redirection, one must be cognizant of the process context in which these functions will execute as the results may be different depending on whether the process is a native 64-bit process or a WoW64 process. On the other hand, one may explicitly specify the `KEY_WOW64_64KEY` or `KEY_WOW64_32KEY` in the API calls to remove any ambiguity.

### Offline Registry API

As mentioned in a previous item, the Windows registry is persisted across system reboots by way of registry hive files stored on disk. The Configuration Manager then consumes this set of hive files to construct the logical view of the registry that is exposed to applications during system operation. However, there are certain circumstances under which it is necessary to manipulate the data stored in the registry in an offline manner; that is, read and write registry keys and values without loading them into the active registry. Such functionality is made available to developers through the offline registry API. The offline registry API is implemented in the _offreg.dll_ dynamic link library; one need only statically link against this library to make use of the functions exposed by the offline registry API.

The offline registry API is powerful for a number of reasons. Aside from the obvious utility of being able to read and write registry contents outside of the active registry, the offline registry API makes registry operations accessible to standard users because the only permissions required to use the API are read (and possibly write) access to the hive file. Under normal conditions, standard user access to the active registry is far more limited.

One may open an existing registry hive file via the `OROpenHive()` function. Alternatively, one may open an empty instance of a registry hive via the `ORCreateHive()` function. Once a handle to the open registry hive is obtained, one may operate on the hive in a way that is perfectly analogous to the live registry API:
- `OROpenKey()`: opens an existing offline registry key
- `ORCreateKey()`: creates a new offline registry key
- `OREnumKey()`: enumerates subkeys of a specified offline registry key
- `OREnumValue()`: enumerates values of a specified offline registry key
- `ORGetValue()`: gets the data and type for the specified value under the associated offline registry key
- `ORSetValue()`: set the data and type for the specified value under the associated offline registry key
- `ORDeleteKey()`: deletes an existing offline registry key 

### References

- _Windows Internals, 6th Edition Part 1_ Pages 277-305
