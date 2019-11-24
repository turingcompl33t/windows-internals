## Interprocess Communication (IPC)

A survey of the various IPC mechanisms available on Windows platforms.

### Anonymous Pipes

Anonymous pipes are perhaps the simplest IPC mechanism available. They provide half-duplex, byte-based IPC.

An anonymous pipe always has two (2) handles associated with it: a read handle and a write handle. Create a new anonymous pipe with the `CreatePipe()` function. 

### Named Pipes

Named pipes are significantly more powerful IPC mechanisms than their anonymous brethren. Characteristics include:

- Message-oriented communication, rather than simply byte-oriented
- Full-duplex (bidirectional)
- Support for multiple, independent instances of pipes with the same name
- Accessible over a network, owing to the fact that the pipe has an identifier

Create a new named pipe with the `CreateNamedPipe()` function. Note that, as mentioned above, multiple independent instances of a pipe with the same name are permitted. This implies that the first call to `CreateNamedPipe()` actually creates the named pipe AND opens an instance of the pipe. Subsequent calls to this function with the same pipe name specified as an argument will only open a new instance of the existing pipe. The pipe is deleted when the last instance of the pipe is closed.

**Status APIs**

- `GetNamedPipeHandleState()`
- `SetNamedPipeHandleState()`
- `GetNamedPipeInfo()`
- `GetNamedPipeClientSessionId()`
- `GetNamedPipeServerSessionId()`
- `GetNamedPipeClientProcessId()`
- `GetNamedPipeServerProcessId()`

**Communications APIs**

- `ConnectNamedPipe()`
- `DisconnectNamedPipe()`
- `ReadFile()`
- `WriteFile()`

**Transaction APIs**

- `TransactNamedPipe()`
- `CallNamedPipe()`

### Mailslots

TODO.