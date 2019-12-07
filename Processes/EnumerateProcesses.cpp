// EnumerateProcesses.cpp
// Demonstration of various methods for enumerating processes.

#include <windows.h>
#include <Psapi.h>
#include <cstdio>

#include <memory>

VOID EnumerateProcessesWin32();

INT wmain(VOID)
{
    EnumerateProcessesWin32();
}

// use the win32 API to enumerate processes
VOID EnumerateProcessesWin32()
{
    DWORD dwActualSize;
    DWORD dwMaxCount = 256;
    DWORD dwCount = 0;

    std::unique_ptr<DWORD[]> pPids;

    for (;;)
    {   
        pPids = std::make_unique<DWORD[]>(dwMaxCount);
        if (!::EnumProcesses(pPids.get(), dwMaxCount*sizeof(DWORD), &dwActualSize))
        {
            break;
        }

        dwCount = dwActualSize / sizeof(DWORD);
        if (dwCount < dwMaxCount)
        {
            break;
        }

        // resize and try again
        dwMaxCount *= 2;
    }

    // extremely lame output...
    for (DWORD i = 0; i < dwCount; ++i)
    {
        printf("PID: %5u\n", pPids[i]);
    }
}