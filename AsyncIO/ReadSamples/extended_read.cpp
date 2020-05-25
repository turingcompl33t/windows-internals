// extended_read.cpp
//
// Demonstration of asynchronous IO via extended IO with completion routines
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 extended_read.cpp

#include <windows.h>

#include <cstdio>
#include <string>
#include <memory>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

// size of individual read operations
constexpr const auto CHUNKSIZE = 4096;

// total number of operations completed
// there are better ways to maintain this state
static unsigned long long ops_completed = 0;

// callback function invoked by IO completion
static void APIENTRY on_read_complete(
    unsigned long /* error_code */,
    unsigned long bytes_xfer,
    LPOVERLAPPED  ov
)
{
    auto offset = LARGE_INTEGER{};
    offset.LowPart  = ov->Offset;
    offset.HighPart = ov->OffsetHigh;

    printf("[Read Complete]\n\tOffset: %llu\n\tBytes: %u\n",
        offset.QuadPart,
        bytes_xfer);

    ++ops_completed;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("[-] Error: invalid arguments\n");
        printf("[-] Usage: %s <FILENAME>\n", argv[0]);
        return STATUS_FAILURE_I;
    }

    // acquire a handle to the target file
    // specify the overlapped flag in file creation call
    auto file = ::CreateFileA(
        argv[1],
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr);

    if (INVALID_HANDLE_VALUE == file)
    {
        printf("[-] Failed to open file\n");
        printf("[-] GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    auto file_size = LARGE_INTEGER{};

        // query the file size
    if (!::GetFileSizeEx(file, &file_size))
    {
        printf("[-] Failed to query file size\n");
        printf("[-] GLE: %u\n", ::GetLastError());
        ::CloseHandle(file);
        return STATUS_FAILURE_I;
    }

    // compute the size of individual read operations

    unsigned long long const n_operations 
        = file_size.QuadPart / CHUNKSIZE + 1;

    // allocate a buffer into which file contents may be read
    // we don't actually do anything with the contents, but one may
    // imagine that arbitrary processing may be performed once
    // the file contents are available after the IO completes
    auto buffer = std::make_unique<char[]>(file_size.QuadPart);
    
    // allocate an array of overalapped contexts, one for each operation
    auto contexts = std::make_unique<OVERLAPPED[]>(n_operations);

    // track current position in the file
    auto position = LARGE_INTEGER{};

    // issue the read operations
    for (auto i = 0; i < n_operations; ++i)
    {
        contexts[i].Offset     = position.LowPart;
        contexts[i].OffsetHigh = position.HighPart;

        auto res = ::ReadFileEx(
            file,
            static_cast<char*>(buffer.get())
                + position.QuadPart,
            CHUNKSIZE,
            &contexts[i],
            on_read_complete);

        if (!res && ::GetLastError() != ERROR_IO_PENDING)
        {
            printf("[-] Failed to initiate asynchronous read\n");
            printf("[-] GLE: %u\n", ::GetLastError());
            ::CloseHandle(file);
            return STATUS_FAILURE_I;
        }

        position.QuadPart += CHUNKSIZE;
    }

    // wait for all operations to complete
    while (ops_completed < n_operations)
    {
        // enter alertable wait state
        ::SleepEx(INFINITE, TRUE);
    }

    ::CloseHandle(file);

    return STATUS_SUCCESS_I;
}