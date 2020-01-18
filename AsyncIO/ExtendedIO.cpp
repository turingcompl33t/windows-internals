// ExtendedIO.cpp
// Demonstration of asynchronous IO via extended IO with completion routines.

#include <windows.h>
#include <cstdio>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

constexpr auto N_OPERATIONS = MAXIMUM_WAIT_OBJECTS;

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

VOID APIENTRY CompletionCallback(
    DWORD dwErrorCode,
    DWORD nBytesTransferred,
    LPOVERLAPPED lpOverlapped
);

// total number of operations completed
static ULONGLONG completed = 0;

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
        contexts[i].Offset     = position.LowPart;
        contexts[i].OffsetHigh = position.HighPart;

        ::ReadFileEx(
            hFile,
            static_cast<CHAR*>(buffer)
                + position.QuadPart,
            readSize,
            &contexts[i],
            CompletionCallback
        );

        position.QuadPart += readSize;
    }

    // wait for all operations to complete
    while (completed < N_OPERATIONS)
    {
        // enter alertable wait state
        ::SleepEx(INFINITE, TRUE);
    }

    ::HeapFree(::GetProcessHeap(), NULL, buffer);
    ::CloseHandle(hFile);

    return STATUS_SUCCESS_I;
}

// callback function invoked by IO completion
static VOID APIENTRY CompletionCallback(
    DWORD dwErrorCode,
    DWORD nBytesTransferred,
    LPOVERLAPPED lpOverlapped
)
{
    LARGE_INTEGER offset{};
    offset.LowPart  = lpOverlapped->Offset;
    offset.HighPart = lpOverlapped->OffsetHigh;

    printf("[Read Complete]\n\tOffset: %llu\n\tBytes: %u\n",
        offset.QuadPart,
        nBytesTransferred
        );

    completed++;
}