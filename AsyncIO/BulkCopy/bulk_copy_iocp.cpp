// bulk_copy_iocp.cpp
//
// Bulk filesystem copy with IO completion port.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include bulk_copy_iocp.cpp

#include <windows.h>

#include <string>
#include <atomic>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <optional>
#include <iostream>

#include <wdl/debug.hpp>
#include <wdl/handle.hpp>
#include <wdl/io/iocp.hpp>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto CHUNKSIZE = 1 << 16;

using invalid_handle = wdl::handle::invalid_handle;

enum class CompletionKey
{
    read,
    write
};

struct FilePair
{
    std::string src;
    std::string dst;

    wdl::handle::invalid_handle src_handle;
    wdl::handle::invalid_handle dst_handle;

    unsigned long long size;

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
};

[[nodiscard]]
unsigned long long register_all(
    wdl::io::iocp&         port,
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
        
        // add both the src and dst files to the completion port
        wdl::io::register_io(port, src.get(), (UINT_PTR)CompletionKey::read);
        wdl::io::register_io(port, dst.get(), (UINT_PTR)CompletionKey::write);

        pair.size       = size.QuadPart;
        pair.src_handle = std::move(src);
        pair.dst_handle = std::move(dst);
        
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

        ::ZeroMemory(io_context, sizeof(OVERLAPPED));
        
        auto status = ::ReadFile(
            pair.src_handle.get(),
            io_context->buffer.get(),
            CHUNKSIZE,
            nullptr,
            io_context);

        ASSERT(!status && ::GetLastError() == ERROR_IO_PENDING);
    }
}

void worker(
    wdl::io::iocp&        port,
    std::atomic_uint64_t& count
    )
{
    while (count.load() > 0)
    {
        auto bytes_xfer = unsigned long{};
        
        auto key = ULONG_PTR{};
        auto ov  = LPOVERLAPPED{};

        // wait for an IO completion 
        if (!wdl::io::get_completion(port, &bytes_xfer, &key, &ov, 100))
        {
            if (::GetLastError() != WAIT_TIMEOUT)
            {
                delete ov;
                --count;
            }
            continue;
        }

        auto io = static_cast<IoContext*>(ov);

        if (key == (ULONG_PTR)CompletionKey::read)
        {
            auto offset = ULARGE_INTEGER{io->Offset, io->OffsetHigh};
            offset.QuadPart += CHUNKSIZE;

            // determine if read operations remain to be completed
            if (offset.QuadPart < io->size)
            {
                auto new_io = new IoContext;
                new_io->size = io->size;
                new_io->buffer = std::make_unique<char[]>(CHUNKSIZE);
                new_io->src_handle = io->src_handle;
                new_io->dst_handle = io->dst_handle;
                ::ZeroMemory(new_io, sizeof(OVERLAPPED));
                new_io->Offset = offset.LowPart;
                new_io->OffsetHigh = offset.HighPart;

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
            auto success = ::WriteFile(
                io->dst_handle,
                io->buffer.get(),
                bytes_xfer,
                nullptr,
                ov);

            ASSERT(!success && ::GetLastError() == ERROR_IO_PENDING);
        }
        else
        {
            // write operation completed
            --count;
            delete io;
        }
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

    // create a completion port
    auto port = wdl::io::iocp{threadcount};
    if (!port)
    {
        std::cout << "Failed to create IOCP\n";
        return STATUS_FAILURE_I;
    }

    // register the file handle with the port
    auto raw_count = register_all(port, files);
    
    auto atomic_count = std::atomic_uint64_t{raw_count};

    // initialize the threadpool
    auto threadpool = std::vector<std::thread>{};
    for (auto i = 0u; i < threadcount; ++i)
    {
        threadpool.emplace_back(worker, std::ref(port), std::ref(atomic_count));
    }

    auto start = std::chrono::steady_clock::now();

    // initate the read operations
    initiate_all(files);

    // wait for threads to complete
    for (auto& t : threadpool)
    {
        t.join();
    }

    auto stop = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

    std::cout << "Completed in " << elapsed.count() << " ms\n";

    return STATUS_SUCCESS_I;
}