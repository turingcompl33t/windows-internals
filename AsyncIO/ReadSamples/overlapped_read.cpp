// overlapped_read.cpp
//
// Demonstration of overlapped IO
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 overlapped_read.cpp

#include <windows.h>

#include <cstdio>
#include <string>
#include <memory>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

// size of individual read operations
constexpr const auto CHUNKSIZE = 4096;

static unsigned long long ops_completed = 0;

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
        nullptr
    );

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

    // compute the necessary number of read operations
    unsigned long long const n_operations 
        = file_size.QuadPart / CHUNKSIZE + 1;

    // we could easily scale the chunksize to handle these cases,
    // but not going to bother for this simple example
    if (n_operations > MAXIMUM_WAIT_OBJECTS)
    {
        printf("[-] Greater than MAXIMUM_WAIT_OBJECTS operations required, sorry!\n");
        ::CloseHandle(file);
        return STATUS_FAILURE_I;
    }

    // allocate a buffer into which file contents may be read
    // we don't actually do anything with the contents, but one may
    // imagine that arbitrary processing may be performed once
    // the file contents are available after the IO completes
    auto buffer = std::make_unique<char[]>(file_size.QuadPart);

    auto contexts = std::make_unique<OVERLAPPED[]>(n_operations);
    auto events   = std::make_unique<HANDLE[]>(n_operations);
    
    // track current position in the file
    auto position = LARGE_INTEGER{};

    // issue the read operations
    for (auto i = 0; i < n_operations; ++i)
    {        
        auto ev = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
        
        events[i] = ev;

        contexts[i].hEvent     = ev;
        contexts[i].Offset     = position.LowPart;
        contexts[i].OffsetHigh = position.HighPart;

        // issue the overlapped read operation
        auto res = ::ReadFile(
            file,
            static_cast<char*>(buffer.get()) 
                + position.QuadPart,
            CHUNKSIZE,
            nullptr,
            &contexts[i]);

        if (!res && ::GetLastError() != ERROR_IO_PENDING)
        {
            printf("[-] Failed to initiate asynchronous read\n");
            printf("[-] GLE: %u\n", ::GetLastError());
            ::CloseHandle(file);
            return STATUS_FAILURE_I;
        }

        position.QuadPart += CHUNKSIZE;
    }

    // continue waiting for results until all have completed
    while (ops_completed < n_operations)
    {
        auto bytes_read = unsigned long{};
        auto offset     = LARGE_INTEGER{};

        auto i = ::WaitForMultipleObjects(
            static_cast<unsigned long>(n_operations),  // explicit narrow
            events.get(),
            FALSE,
            INFINITE);
        
        if (i == WAIT_FAILED)
        {
            printf("[-] Something went wrong...\n");
            break;
        }

        // compute the index of the operation that completed
        auto index = i - WAIT_OBJECT_0;

        ::GetOverlappedResult(
            file,
            &contexts[index],
            &bytes_read,
            FALSE
            );

        offset.LowPart  = contexts[index].Offset;
        offset.HighPart = contexts[index].OffsetHigh;

        printf("[Read Complete]\n\tOffset: %llu\n\tBytes:  %u\n",
            offset.QuadPart,
            bytes_read
            );

        ops_completed++;
    }

    ::CloseHandle(file);

    return STATUS_SUCCESS_I;
}