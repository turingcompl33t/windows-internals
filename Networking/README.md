### Windows sockets

- Windows sockets 2 (Winsock2) is the current windows socket API implementation
- Winsock2 is backwards compatible with Winsock1.1
- Utilizes the socket paradigm popularized by BSD Unix sockets (Berkeley sockets)
- WSA = windows socket API
- Latest update of WSA was for 8.1/2008 R2
- Winsock DLL = ws2_32.dll
- Need to link with ws2_32.lib
    - E.g. #pragma comment(“lib”, “ws2_32.lib”)

### Berkeley sockets vs Windows sockets

- Minimal areas where windows socket API deviates from BSD 
- Initialization
    - In UNIX, no explicit socket initialization is needed before sockets are created 
    - In Windows, the first socket call made by application must be WSAStartup() to initialize winsock
    - Also must call WSACleanup() to cleanup the resources initialized by the winsock DLL
- In windows, a socket descriptor IS NOT a file descriptor and thus must only be used with winsock functions, whereas in UNIX socket descriptors are just file descriptors
    - Thus in UNIX we can use any file IO functions to interact with sockets as well
- In UNIX, a socket descriptor is a small, nonnegative integer
    - In Windows, it may be any handle value, so long as it is not INVALID_SOCKET
    - We cannot make the same assumptions on Windows as we may on UNIX
- Implementation of select() function changed in Windows from UNIX
    - This is because of the aforementioned fact that Windows sockets are not necessarily small, nonnegative integers
    - In UNIX, argument to select is a bitmask, under the opaque type fd_set
    - On Windows, fd_set is implemented as an array of socket handles
    - Thus we need to use a different macro FD_XXX when using select() on windows
- Error code retrieval 
    - In winsock, use WSAGetLastError() which is the socket substitute for GetLastError() from Win32
    - The error constants on Windows are redefinitions of the constants from UNIX
    - Same semantic meanings, but different names
- Function renaming
    - On windows, sockets must be closed via CloseSocket() whereas on UNIX we call Close() (this is because of fd thing)
    - On windows need to use ioctlsocket() and WSAIoctl() functions to perform IO control, whereas on UNIX we just call ioctl() (system call)
- Header files
    - On windows, just include <Winsock2>
    - Whereas on UNIX we include all kinds of crazy files and whatnot
- Return values on error
    - On windows, use the SOCKET_ERROR constant to check return values for error condition

In summary:

“Moving from UNIX sockets to Windows sockets is fairly simple. Windows programs require a different set of include files, need initialization and deallocation of WinSock resources, use closesocket( ) instead of close( ), and use a different error reporting facility. However, the meat of the application is identical to UNIX.”
