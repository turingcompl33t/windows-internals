// SystemInfo.cpp
// Using GetSystemInfo to view memory allocation granularity.

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <cstdio>

INT wmain(VOID)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    auto bytes  = info.dwAllocationGranularity;
    auto kbytes = bytes / (1 << 10);

    printf("dwAllocationGranularity: %u Bytes (%u KB)\n", bytes, kbytes);

    return 0;
}