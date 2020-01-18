// OverlappedIO.cpp
// Demonstration of overlapped IO.
//
// Build:
//  cl /EHsc /nologo /std:c++17 OverlappedIO.cpp

#include <windows.h>
#include <cstdio>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

constexpr auto N_OPERATIONS = MAXIMUM_WAIT_OBJECTS;

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

INT wmain(INT argc, WCHAR* argv[])
{
    if (argc != 2)
    {
        printf("[-] Error: invalid arguments\n");
        printf("[-] Usage: %ws <FILENAME>\n", argv[0]);
        return STATUS_FAILURE_I;
    }

    HANDLE        hFile;
    LARGE_INTEGER fileSize{};
    ULONGLONG     readSize{};

    ULONGLONG  completed{};
    OVERLAPPED contexts[N_OPERATIONS];
    HANDLE     events[N_OPERATIONS];

    if (!fs::is_regular_file(argv[1]))
    {
        printf("[-] Invalid input file specified\n");
        return STATUS_FAILURE_I;
    }

    // acquire a handle to the target file
    // specify the overlapped flag in file creation call
    hFile = ::CreateFileW(
        argv[1],
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr
    );

    if (INVALID_HANDLE_VALUE == hFile)
    {
        printf("[-] Failed to open file\n");
        printf("[-] GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    // query the file size
    if (!::GetFileSizeEx(hFile, &fileSize))
    {
        printf("[-] Failed to query file size\n");
        printf("[-] GLE: %u\n", ::GetLastError());
        ::CloseHandle(hFile);
        return STATUS_FAILURE_I;
    }

    // compute the size of individual read operations
    readSize = fileSize.QuadPart / N_OPERATIONS;

    // allocate a buffer into which file contents may be read
    // we don't actually do anything with the contents, but one may
    // imagine that arbitrary processing may be performed once
    // the file contents are available after the IO completes
    auto buffer = ::HeapAlloc(::GetProcessHeap(), NULL, fileSize.QuadPart);
    if (NULL == buffer)
    {
        printf("[-] Failed to allocate read buffer\n");
        printf("[-] GLE: %u\n", ::GetLastError());
        ::CloseHandle(hFile);
        return STATUS_FAILURE_I;
    }
    
    // track current position in the file
    LARGE_INTEGER position{};
    position.QuadPart = 0;

    // issue the read operations
    for (auto i = 0; i < N_OPERATIONS; ++i)
    {
        HANDLE hEvent = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
        
        events[i] = hEvent;

        contexts[i].hEvent     = hEvent;
        contexts[i].Offset     = position.LowPart;
        contexts[i].OffsetHigh = position.HighPart;

        // issue the overlapped read operation
        ::ReadFile(
            hFile,
            static_cast<CHAR*>(buffer) 
                + position.QuadPart,
            readSize,
            nullptr,
            &contexts[i]
        );

        position.QuadPart += readSize;
    }

    // continue waiting for results until all have completed
    while (completed < N_OPERATIONS)
    {
        DWORD         nBytesRead;
        LARGE_INTEGER offset{};

        auto i = ::WaitForMultipleObjects(
            N_OPERATIONS,
            events,
            FALSE,
            INFINITE
            );
        
        if (i == WAIT_FAILED)
        {
            printf("[-] Something went wrong...\n");
            break;
        }

        // compute the index of the operation that completed
        auto index = i - WAIT_OBJECT_0;

        ::GetOverlappedResult(
            hFile,
            &contexts[index],
            &nBytesRead,
            FALSE
            );

        offset.LowPart  = contexts[index].Offset;
        offset.HighPart = contexts[index].OffsetHigh;

        printf("[Read Complete]\n\tOffset: %llu\n\tBytes:  %u\n",
            offset.QuadPart,
            nBytesRead
            );

        completed++;
    }

    ::HeapFree(::GetProcessHeap(), NULL, buffer);
    ::CloseHandle(hFile);

    return STATUS_SUCCESS_I;
}