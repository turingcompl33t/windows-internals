// bulk_copy_threadpool.cpp
//
// Bulk filesystem copy with the Windows threadpool.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include bulk_copy_threadpool.cpp

#include <windows.h>

#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>
#include <iostream>

#include <wdl/debug.hpp>
#include <wdl/handle.hpp>
#include <wdl/threadpool/pool.hpp>
#include <wdl/synchronization/event.hpp>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto CHUNKSIZE = 1 << 16;

using event          = wdl::synchronization::event;
using event_type     = wdl::synchronization::event_type;
using invalid_handle = wdl::handle::invalid_handle;

event                g_done_event{event_type::manual_reset};
std::atomic_uint64_t g_operation_count{};

struct FilePair
{
    std::string src;
    std::string dst;

    wdl::handle::invalid_handle src_handle;
    wdl::handle::invalid_handle dst_handle;

    unsigned long long size;

    // unique handles to threadpool IO objects
    wdl::handle::io_handle src_io;
    wdl::handle::io_handle dst_io;

    FilePair(std::string src_, std::string dst_)
        : src{src_}, dst{dst_}
    {}
};

struct IoContext : OVERLAPPED
{
    HANDLE                  src_handle;
    HANDLE                  dst_handle;
    std::unique_ptr<char[]> buffer;
    unsigned long long      size;
    PTP_IO                  src_io;
    PTP_IO                  dst_io;
};

void read_completion_handler(
    PTP_CALLBACK_INSTANCE,
    void*,
    void*         ov, 
    unsigned long io_result, 
    ULONG_PTR     bytes_xfer, 
    PTP_IO
    )
{    
    ASSERT(io_result == ERROR_SUCCESS);

    // read operation has completed
    auto io = static_cast<IoContext*>(ov);

    auto offset = ULARGE_INTEGER{io->Offset, io->OffsetHigh};
    offset.QuadPart += CHUNKSIZE;

    // determine if read operations remain to be completed
    if (offset.QuadPart < io->size)
    {
        auto new_io = new IoContext;

        new_io->size       = io->size;
        new_io->buffer     = std::make_unique<char[]>(CHUNKSIZE);
        new_io->src_handle = io->src_handle;
        new_io->dst_handle = io->dst_handle;
        new_io->src_io     = io->src_io;
        new_io->dst_io     = io->dst_io;

        ::ZeroMemory(new_io, sizeof(OVERLAPPED));

        new_io->Offset     = offset.LowPart;
        new_io->OffsetHigh = offset.HighPart;

        wdl::threadpool::start_io(new_io->src_io);

        auto success = ::ReadFile(
            new_io->src_handle,
            new_io->buffer.get(),
            CHUNKSIZE,
            nullptr,
            new_io);

        ASSERT(!success && ::GetLastError() == ERROR_IO_PENDING);
    }

    // read operation completed, initiate write to corresponding location in target
    // offset is the same as for the write, just a new file this time
    io->Internal     = 0;
    io->InternalHigh = 0;

    wdl::threadpool::start_io(io->dst_io);

    auto success = ::WriteFile(
        io->dst_handle,
        io->buffer.get(),
        static_cast<unsigned long>(bytes_xfer),  // explicit narrow
        nullptr,
        io);

    ASSERT(!success && ::GetLastError() == ERROR_IO_PENDING);
} 

void write_completion_handler(
    PTP_CALLBACK_INSTANCE,
    void*,
    void*         ov, 
    unsigned long io_result, 
    ULONG_PTR, 
    PTP_IO
    )
{
    ASSERT(io_result == ERROR_SUCCESS);

    auto io_context = static_cast<IoContext*>(ov);
    delete io_context;
    
    if (--g_operation_count == 0)
    {
        // all operations completed
        g_done_event.set();
    }
} 

[[nodiscard]]
unsigned long long register_all(
    wdl::threadpool::pool& pool,
    std::vector<FilePair>& files
    )
{
    auto count = unsigned long long{};

    for (auto& pair : files)
    {
        invalid_handle src
        {
            ::CreateFileA(
                pair.src.c_str(), 
                GENERIC_READ, 
                FILE_SHARE_READ, 
                nullptr,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED,
                nullptr)
        };

        if (!src) continue;

        // get the file size
        auto size = LARGE_INTEGER{};
        ::GetFileSizeEx(src.get(), &size);

        invalid_handle dst
        {
            ::CreateFileA(
                pair.dst.c_str(),
                GENERIC_WRITE,
                0,
                nullptr,
                OPEN_ALWAYS,
                FILE_FLAG_OVERLAPPED,
                nullptr)
        };

        if (!dst) continue;

        // set the final file size now because operation
        // is always performed synchronously
        ::SetFilePointerEx(dst.get(), size, nullptr, FILE_BEGIN);
        ::SetEndOfFile(dst.get());
        
        // create IO objects for src and dst
        auto src_io = wdl::handle::io_handle { 
            wdl::threadpool::create_io(pool, src.get(), read_completion_handler, nullptr) };
        auto dst_io = wdl::handle::io_handle {
            wdl::threadpool::create_io(pool, dst.get(), write_completion_handler, nullptr) };

        pair.size       = size.QuadPart;
        pair.src_handle = std::move(src);
        pair.dst_handle = std::move(dst);
        pair.src_io     = std::move(src_io);
        pair.dst_io     = std::move(dst_io);
        
        // track the total number of operations required for all operations
        count += (size.QuadPart + CHUNKSIZE - 1) / CHUNKSIZE;
    } 

    return count;
}

void initiate_all(std::vector<FilePair>& files)
{
    for (auto& pair : files)
    {
        // initiate the first read
        auto io_context = new IoContext;

        io_context->size       = pair.size;
        io_context->buffer     = std::make_unique<char[]>(CHUNKSIZE);
        io_context->src_handle = pair.src_handle.get();
        io_context->dst_handle = pair.dst_handle.get();

        // IO context receives non-owning pointers to IO objects
        io_context->src_io     = pair.src_io.get();
        io_context->dst_io     = pair.dst_io.get();

        ::ZeroMemory(io_context, sizeof(OVERLAPPED));
        
        // signal threadpool to initiate IO related to this IO object
        wdl::threadpool::start_io(io_context->src_io);

        // initiate the first read operation
        auto status = ::ReadFile(
            pair.src_handle.get(),
            io_context->buffer.get(),
            CHUNKSIZE,
            nullptr,
            io_context);

        ASSERT(!status && ::GetLastError() == ERROR_IO_PENDING);
    }
}

std::optional<unsigned long> parse_threadcount(char* count_str)
{
    try
    {
        return std::stoul(count_str);
    }   
    catch (std::exception const&)
    {
        return std::nullopt;
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Invalid arguments\n";
        std::cout << "Usage: " << argv[0] << " <THREAD COUNT>\n";
        return STATUS_FAILURE_I;
    }

    // parse threadcount, default to hardware concurrency count
    auto threadcount = parse_threadcount(argv[1])
        .value_or(std::thread::hardware_concurrency());

    // parse command line for filename src and destinations
    auto files = std::vector<FilePair>{};
    auto test_pair = FilePair{"src.txt", "dst.txt"};
    files.push_back(std::move(test_pair));

    // create a user-defined threadpool
    auto pool = wdl::threadpool::pool{};
    if (!pool)
    {
        std::cout << "Failed to create threadpool\n";
        return STATUS_FAILURE_I;
    }
    
    pool.set_max_threadcount(threadcount);

    // register the file handle with the port
    auto raw_count = register_all(pool, files);
    g_operation_count.store(raw_count);
    
    auto start = std::chrono::steady_clock::now();

    // initate the read operations
    initiate_all(files);

    // wait for IO to complete
    g_done_event.wait();

    auto stop = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

    std::cout << "Completed in " << elapsed.count() << " ms\n";

    return STATUS_SUCCESS_I;
}