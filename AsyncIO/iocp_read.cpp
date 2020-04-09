// iocp_read.cpp
//
// Demo of asynchronous read operations via an IO completion port
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 iocp_read.cpp

#include <windows.h>

#include <string>
#include <memory>
#include <iostream>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto CHUNKSIZE = 4096;

struct thread_arg_t
{   
    HANDLE             port;
    unsigned long long n_operations;
};

void error(
    std::string const& msg,
    unsigned long code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << " Code: " << std::hex << code << std::dec
        << std::endl;
}

unsigned long __stdcall worker(void* arg)
{
    auto& ctx = *static_cast<thread_arg_t*>(arg);

    auto bytes_xfer     = unsigned long{};
    auto completion_key = ULONG_PTR{};
    auto p_ov           = LPOVERLAPPED{};

    auto offset = LARGE_INTEGER{};

    auto operations_completed = unsigned long long{};

    while (operations_completed < ctx.n_operations)
    {
        if (!::GetQueuedCompletionStatus(
            ctx.port, 
            &bytes_xfer, 
            &completion_key, 
            &p_ov, 
            INFINITE))
        {
            error("GetQueuedCompletionStatus() failed");
            return STATUS_FAILURE_I;
        }

        // lazy way to determine offset at which operation completed
        offset.LowPart  = p_ov->Offset;
        offset.HighPart = p_ov->OffsetHigh;

        // read operation completed
        std::cout << "[Read Completed]\n"
            << "\tOffset: " << offset.QuadPart
            << "\n\tBytes: " << bytes_xfer
            << std::endl;

        ++operations_completed;
    }

    return STATUS_SUCCESS_I;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "[-] Invalid arguments\n";
        std::cout << "[-] Usage: " << argv[0] << " <FILENAME>\n";
        return STATUS_FAILURE_I;
    }

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
        error("CreateFile() failed");
        return STATUS_FAILURE_I;
    }

    // query the size of the specified file
    auto file_size = LARGE_INTEGER{};

    if (!::GetFileSizeEx(file, &file_size))
    {
        error("GetFileSizeEx() failed");
        ::CloseHandle(file);
        return STATUS_FAILURE_I;
    }

    // create the IO completion port
    auto port = ::CreateIoCompletionPort(
        INVALID_HANDLE_VALUE, 
        nullptr, 
        0, 1);
    if (NULL == port)
    {
        error("CreateIoCompletionPort() failed (creation)");
        ::CloseHandle(file);
        return STATUS_FAILURE_I;
    }

    // associate the file handle with the port
    if (::CreateIoCompletionPort(file, port, 1, 0) == NULL)
    {
        error("CreateIoCompletionPort() failed (adding file handle)");
        ::CloseHandle(port);
        ::CloseHandle(file);
        return STATUS_FAILURE_I;
    }

    // allocate a buffer for read operations
    auto buffer = std::make_unique<char[]>(file_size.QuadPart);

    // compute the number of operations required to read the entire file
    unsigned long long const n_operations 
        = file_size.QuadPart / CHUNKSIZE + 1;

    // setup read operation contexts
    auto contexts = std::make_unique<OVERLAPPED[]>(n_operations);

    // prepare an argument for the thread to access necessary context
    auto thread_arg = thread_arg_t{};

    thread_arg.port         = port;
    thread_arg.n_operations = n_operations;

    // start a new thread to service the completion port
    auto thread = ::CreateThread(nullptr, 0, worker, &thread_arg, 0, nullptr);

    // issue the read operations
    auto position = LARGE_INTEGER{};
    for (auto i = 0u; i < n_operations; ++i)
    {
        contexts[i].Offset     = position.LowPart;
        contexts[i].OffsetHigh = position.HighPart;

        auto res = ::ReadFile(
            file,
            static_cast<char*>(buffer.get())
                + position.QuadPart,
            CHUNKSIZE,
            nullptr,
            &contexts[i]);

        if (!res && ::GetLastError() != ERROR_IO_PENDING)
        {
            error("ReadFile() failed");
            ::CloseHandle(file);
            return STATUS_FAILURE_I;
        }

        position.QuadPart += CHUNKSIZE;
    }

    // wait for the thread to complete
    ::WaitForSingleObject(thread, INFINITE);

    ::CloseHandle(thread);
    ::CloseHandle(port);
    ::CloseHandle(file);

    return STATUS_SUCCESS_I;
}