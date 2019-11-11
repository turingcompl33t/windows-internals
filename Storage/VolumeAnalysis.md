## Windows Volume Analysis

The intermediary layer between raw disk bytes and filesystem-level analysis.

### Terminology

- _Partition_: a consecutive group of sectors on a physical disk
- _Volume_: a set of sectors (not necessarily consecutive) that may be used for storage

### Windows Disk Configurations

**Basic Disks**

Basic disks exist in one of two configurations:

- Four primary partitions each formatted with its own filesystem
- Three primary partitions and one extended partition which may consist of up to 128 logical drives, each of which may be formatted with its own filesystem

In the latter case, each primary partition and logical drive is referred to as a _basic volume_. 

**Dynamic Disks**

A dynamic disk is more flexible than a basic disk. It may be formatted with up to 2,000 dynamic volumes, each of which may host its own filesystem. In this case, the dynamic volumes are known as _simple volumes_. 

Alternatively, multiple physical dynamic disks may be combined to implement a single, larger dynamic volume which hosts only a single filesystem. Under this configuration, individual volumes may fall into one of the three following categories:

- _striped volume_
- _mirrored volume_
- _spanned volume_

### Raw Disk Access

One may perform direct disk IO from userspace (provided administrator-level privileges) via the `CreateFile()` API. See the MS KB article referenced below

### References

- _The Rootkit Arsenal_ Pages 289-297
- [MS Knowledge Base 100027](https://support.microsoft.com/en-ca/help/100027/info-direct-drive-access-under-win32)