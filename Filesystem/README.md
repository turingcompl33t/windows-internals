## Windows Filesystem Management

The basics of filesystem management on Windows systems.

### Obtaining File Handles

File handles may be obtained via a variety of APIs. One may open an existing file with the `OpenFile()` function. However, this function's functionality is limited and is completely subsumed by the more general `CreateFile()`, thus it is recommended to use `CreateFile()` in all new applications.

### File Pointers

Like UNIX systems, Windows NTFS supports the concept of file pointers that determine the offset into a file at which IO operations will occur. Once a handle to a file has been obtained, the file pointer may be manually set via the `SetFilePointerEx()` function.

The `SetFilePointerEx()` function accepts a `LARGE_INTEGER` argument that specifies the movement of the file pointer within the file. There are several distinct _move methods_ which determine the semantics of this argument:

- `FILE_BEGIN`: the file pointer is positioned from the start of the file; the argument is interpreted as an unsigned value
- `FILE_CURRENT`: the file pointer is positioned from the current file pointer position; the argument is interpreted as a signed value
- `FILE_END`: the file pointer is positioned from the end of the file; the argument is interpreted as a signed value

The `SetEndOfFile()` function resizes the file using the current position of the file pointer as the length.

**Using Overlapped Structures to Achieve Random-Access File IO**

Setting the file pointer manually via `SetFilePointerEx()` prior to each read/write operation on a file can prove unweildly. An alternative is to specify an `OVERLAPPED` structure in the call to `ReadFile()` or `WriteFile()` which specifies the byte offset at which the IO operation should occur. Essentially, this combines the two steps of setting the file pointer and performing the IO operation into a single step.

### File Size

The size of a file may be queried in a number of ways.

One method to determine file size is to use `SetFilePointerEx()`, specifying the `FILE_END` movement method and a value of 0 for the number of bytes to move within the file. This will return the current file size in bytes.

Another method is to use an explicit call to `GetFileSizeEx()`. 

### References

- _Windows System Programming, 4th Edition_ Pages 59-86