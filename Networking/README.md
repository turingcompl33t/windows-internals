## Windows sockets

The Windows socket programming API.

### Windows Sockets Overview

Windows sockets are the low-level mechanism exposed by the operating system to allow user-mode applications to implement network communication. The Windows sockets API was developed as an extension of the Berkeley Sockets API and is designed to interoperate with applications written using Berkeley sockets.

Windows sockets 2 (Winsock2) is the current windows socket API implementation. This current implementation remains backwards compatible with the previous API implementation - Winsock1.1. The latest update of the Windows sockets API was released with Windows 8.1 and Windows Server 2008 R2.

Making use of the Windows sockets API in application code requires the following:
- Linking with the Windows sockets 2 static library _ws2_32.lib_ (e.g. `#pragma comment("lib", "ws2_32.lib")`)
- Access to the Windows sockets 2 dynamic link library, _ws2_32.dll_, at runtime 

The Windows sockets 2 API may be easily identified in source code via the "WSA" prefix on types and functions. This prefix is an acronym that stands for "Windows Sockets Asynchronous", reflecting the fact that the underlying IO implementation utilized by Windows sockets is designed to be asynchronous.

### Berkeley sockets vs Windows sockets

The Windows sockets API was modeled on the socket API popularized by Berkeley sockets. Furthermore, applications built with Windows sockets are designed to interoperate with applications utilizing Berkeley sockets. Nevertheless, there remain some important distinctions between Windows sockets and Berkeley sockets that application developers must account for when implementing network communications under the Windows sockets API. In the following enumeration, I use the term "UNIX" to refer to applications written using the Berkeley sockets API.

- Initialization
    - In UNIX, no explicit socket initialization is needed before sockets are created 
    - In Windows, the first socket call made by application must be `WSAStartup()` to initialize Winsock
    - Additionally, Windows applications must call `WSACleanup()` to cleanup the resources initialized by the winsock DLL
- In Windows, a socket descriptor IS NOT a file descriptor and thus must only be used with winsock functions, whereas in UNIX socket descriptors are just file descriptors
    - Thus in UNIX one may use any file IO functions to interact with sockets, whereas this flexibility is not offered under Windows
- Socket Descriptor Representation
    - In UNIX, a socket descriptor is a small, nonnegative integer
    - In Windows, it may be any handle (`HANDLE`) value, so long as it is not `INVALID_SOCKET`
    - Thus, one cannot make the same assumptions on Windows as may be made on UNIX regarding the underlying representation of the socket
- Implementation of `select()`
    - The implementation of the commonly used `select()` function is distinct under Windows and UNIX because of the aforementioned fact that Windows sockets are not necessarily small, nonnegative integers
    - In UNIX, argument to `select()` is a bitmask, under the opaque type `fd_set`
    - On Windows, `fd_set` is implemented as an array of socket handles
    - Thus one needs use a different macro `FD_XXX` when using `select()` on Windows
- Error Code Retrieval 
    - In winsock, one uses `WSAGetLastError()` (essentially the socket substitute for `GetLastError()` from Win32) to retrieve error codes from failed calls into the Windows sockets API
    - The error constants on Windows are redefinitions of the constants from UNIX
    - The error codes retain their original semantic meanings, but they are identified by different names
- Function Renaming
    - On Windows, sockets must be closed via `CloseSocket()` whereas on UNIX one calls `close()` (because the socket is represented as a standard file descriptor)
    - On Windows one must use `ioctlsocket()` and `WSAIoctl()` functions to perform IO control operations, whereas on UNIX one simply calls the  standard `ioctl()` system call directly (one could argue this is a result of the larger distinction between the way system calls and the system call API are implemented on Windows and UNIX)
- Header Files
    - On Windows, one just includes the `<Winsock2>` header
    - On UNIX one must include a variety of header files provided by the system in order to work with sockets - the exact headers are dependent upon the needs of the application
- Return Values on Error
    - On Windows, one uses the `SOCKET_ERROR` constant to check return values for error condition

Despite the length of the above list of differences between Windows and UNIX sockets, it is possible that `WSAStartup()` and `WSACleanup()` are the only nonstandard (i.e. not utilized in a Berkely sockets application) functions used in a Windows socket application. Thus, it is a simple matter to use a preprocessor definition to test for the current platform and include or exclude these calls accordingly to write cross-platform networking code under the sockets paradigm.

In summary:

"Moving from UNIX sockets to Windows sockets is fairly simple. Windows programs require a different set of include files, need initialization and deallocation of WinSock resources, use `closesocket( )` instead of `close()`, and use a different error reporting facility. However, the meat of the application is identical to UNIX."

### Windows Sockets API

Aside from the distinctions mentioned above, the Windows sockets API is identical to the Berkeley sockets API.

- `socket()`: create a new socket
- `bind()`: bind an existing socket to a local address
- `listen()`: make a bound socket available for client connections
- `accept()`: wait for a client to connect (may be nonblocking)
- `connect()`: make a client connection to a server socket
- `send()`: send data
- `recv()`: receive data
- `shutdown()`: disable send, receive, or both, but do not free resources
- `closesocket()`: free resources associated with the socket

In addition to the `send()` and `recv()` functions mentioned above, the Win32 functions `ReadFile()` and `WriteFile()` may also be used to transmit data over a socket connection by casting the `SOCKET` to a `HANDLE` in the function call.

For datagram sockets, the familiar `sendto()` and `recvfrom()` calls are used.
