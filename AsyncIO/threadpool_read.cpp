// threadpool_read.cpp
//
// Asynchronous read operations with the Windows threadpool
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 threadpool_read.cpp

#include <windows.h>

#include <memory>
#include <string>
#include <iostream>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto CHUNKSIZE = 4096;

struct io_context_t : OVERLAPPED
{
    HANDLE                  file;
    std::unique_ptr<char[]> buffer;
    unsigned long long      total_size;
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

// invoked by a threadpool thread on IO completion
void __stdcall on_io_complete(
    PTP_CALLBACK_INSTANCE /* instance */,
    void*                 /* context */,
    void*                 overlapped,
    unsigned long         io_result,
    ULONG_PTR             bytes_xfer,
    PTP_IO                io_object
    )
{
    // extract our io context
    auto* io_context = static_cast<io_context_t*>(overlapped);

    if (ERROR_SUCCESS == io_result)
    {
        // the offset at which the read was initiated
        auto offset = ULARGE_INTEGER{ io_context->Offset, io_context->OffsetHigh };

        // read operation completed
        std::cout << "[Read Completed]\n"
            << "\tOffset: " << offset.QuadPart
            << "\n\tBytes: " << bytes_xfer
            << std::endl;

        // compute the new file offset
        offset.QuadPart += CHUNKSIZE;

        // and determine if content remains to be read
        if (offset.QuadPart < io_context->total_size)
        {
            auto new_io_context = new io_context_t{};

            new_io_context->file       = io_context->file;
            new_io_context->buffer     = std::make_unique<char[]>(CHUNKSIZE);
            new_io_context->total_size = io_context->total_size;
            new_io_context->Offset     = offset.LowPart;
            new_io_context->OffsetHigh = offset.HighPart;

            // initiate the next read operation
            ::ReadFile(
                new_io_context->file,
                new_io_context->buffer.get(),
                CHUNKSIZE,
                nullptr,
                static_cast<LPOVERLAPPED>(new_io_context)
                );
            
            // and signal the threadpool that more work is coming its way
            ::StartThreadpoolIo(io_object);
        }
    }
    else
    {
        std::cout << "[Read Completed with Error]" << std::endl;
    }

    // deallocate the old io context
    delete io_context;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "[-] Invalid arguments\n";
        std::cout << "[-] Usage: " << argv[0] << "<FILENAME>\n";
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

    // create the threadpool IO object
    auto io_object = ::CreateThreadpoolIo(file, on_io_complete, nullptr, nullptr);

    // initialize our own io context
    auto io_context = new io_context_t;
    io_context->file       = file;
    io_context->total_size = file_size.QuadPart;
    io_context->buffer     = std::make_unique<char[]>(CHUNKSIZE);
    io_context->Offset     = 0;
    io_context->OffsetHigh = 0;

    // initiate the first read operation
    auto res = ::ReadFile(
        file,
        io_context->buffer.get(),
        CHUNKSIZE,
        nullptr,
        static_cast<LPOVERLAPPED>(io_context)
        );

    // ensure that the operation initiated successfully
    if (!res && ::GetLastError() != ERROR_IO_PENDING)
    {
        error("ReadFile() failed");
        delete io_context;
        ::CloseThreadpoolIo(io_object);
        ::CloseHandle(file);
        return STATUS_FAILURE_I;
    }

    // signal the threadpool to begin processing IO completions 
    ::StartThreadpoolIo(io_object);

    // wait for all pending callbacks to complete, without cancellation
    ::WaitForThreadpoolIoCallbacks(io_object, FALSE);
    ::CloseThreadpoolIo(io_object);

    ::CloseHandle(file);

    return STATUS_SUCCESS_I;
}