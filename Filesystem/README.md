## Windows Filesystem Management

The basics of filesystem management on Windows systems.

### Windows Filesystem Basics


The Windows filesystem API provides application developers with the ability to create, open, read, write, and manipulate the attributes of filesystem files. The filesystem API supports four distinct filesystem implementations:
- NTFS: Microsoft's NT File System
- FAT and FAT32: the File Allocation Table filesystem is frequently used to format removable mass storage devices like flash drives
- CDFS: the CD-ROM filesystem utilizing for reading and writing data to compact disk media (probably OK to ignore this one at this point)
- UDF: the Universal Disk Format filesystem; the updated version of CDFS that also adds support for DVD drives

Because NTFS is the default filesystem implementation utilized by Windows systems it is the primary filesystem of interest for Windows application developers.

The basic API exposed by Windows for creating, reading, writing, and closing files includes the following functions:
- `CreateFile()`: used to create a new file or open an existing file
- `ReadFile()`: use to read content from an open file into the virtual address space of a process
- `WriteFile()`: used to write content from the virtual address space of a process to the underlying filesystem file
- `CloseHandle()`: used to close the file handle obtained via `CreateFile()` and flush any file updates to the underlying filesystem

As the above enumeration indicates, the filesystem API, like the majority of Windows API functions, operates on system resources (like files) in terms of handles to the underlying object implemented in the Executive.

Once a file has been opened for reading or writing, two common operations necessary to interact with the file include querying the file size and setting the file pointer for the current open instance. The Windows API exposes the following functions to achieve this functionality:
- `GetFileSize()`: query the file size
- `GetFileSizeEx()`: an updated version of the `GetFileSize()` function with a more consistent API
- `SetFilePointer()`: sets the file pointer for the currently open instance of the file to a specified location
- `SetFilePointerEx()`: an updated version of the `SetFilePointer()` function with an improved API 

One may utilize the `SetFilePointer[Ex]()` function in conjunction with `ReadFile()` or `WriteFile()` to read or write arbitrary locations in an open file instance. Alternatively, one may utilize an `OVERLAPPED` structure that is provided as a formal parameter to the above functions to achieve the same effect.

The Windows API also defines a number of high-level functions for filesystem management, including the following:
- `DeleteFile()`: deletes an existing file
- `CopyFile()`: copies an existing file to a new file; the new file name may be distinct from the existing name
- `CreateHardLink()`: creates a hard link to an existing file under a specified name; a hard link is simply a distinct filesystem name that references the same underlying file object as the original; changes to the file referenced by one hard link are reflected by all other hard links to the same file
- `CreateSymbolicLink()`: creates a symbolic link to an existing file under a specified name; a symbolic link is merely a pointer to another filesystem object
- `GetFileTime()`: query the creation time, last access time, and last update time for the specified file
- `GetFileAttributes()`: query the attributes for the specified file

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