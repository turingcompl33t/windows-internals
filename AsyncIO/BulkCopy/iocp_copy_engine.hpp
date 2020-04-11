// iocp_copy_engine.cpp
//
// Bulk filesystem copy engine utilizing IO completion port.

#pragma once

#include <windows.h>

#include <string>
#include <atomic>
#include <vector>
#include <memory>
#include <thread>

#include <wdl/debug.hpp>
#include <wdl/handle.hpp>
#include <wdl/io/iocp.hpp>
#include <wdl/synchronization/event.hpp>

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

class iocp_copy_engine
{
    using invalid_handle = wdl::handle::invalid_handle;
    using event          = wdl::synchronization::event;
    using event_type     = wdl::synchronization::event_type;

    unsigned long           m_threadcount;
    unsigned long           m_chunksize;

    wdl::io::iocp            m_port;
    std::vector<FilePair>    m_files;
    std::vector<std::thread> m_threadpool;

    std::atomic_uint64_t     m_operation_count{ 0 };

    event m_finalize{ event_type::manual_reset };

public:
    iocp_copy_engine(
        unsigned long threadcount = std::thread::hardware_concurrency(),
        unsigned long chunksize   = 1 << 16)
        : m_threadcount{ threadcount }
        , m_chunksize{ chunksize }
        , m_port{ threadcount }
    {
        for (auto i = 0u; i < m_threadcount; ++i)
        {
            m_threadpool.emplace_back(&iocp_copy_engine::io_worker, this);
        }
    }

    iocp_copy_engine(iocp_copy_engine const&)            = delete;
    iocp_copy_engine& operator=(iocp_copy_engine const&) = delete;

    iocp_copy_engine(iocp_copy_engine&&)            = delete;
    iocp_copy_engine& operator=(iocp_copy_engine&&) = delete;

    ~iocp_copy_engine() = default;

    void add_file(std::string const& src_file, std::string const& dst_file);

    void finalize();
    void start();

private:
    void io_worker();
};

void iocp_copy_engine::add_file(
    std::string const& src_file,
    std::string const& dst_file
    )
{
    m_files.emplace_back(src_file, dst_file);
}

void iocp_copy_engine::finalize()
{
    for (auto& pair : m_files)
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
        wdl::io::register_io(m_port, src.get(), (UINT_PTR)CompletionKey::read);
        wdl::io::register_io(m_port, dst.get(), (UINT_PTR)CompletionKey::write);

        pair.size       = size.QuadPart;
        pair.src_handle = std::move(src);
        pair.dst_handle = std::move(dst);
        
        // track the total number of operations required for all operations
        m_operation_count.fetch_add((size.QuadPart + m_chunksize - 1) / m_chunksize);
    } 

    m_finalize.set();
}

void iocp_copy_engine::start()
{
    for (auto& pair : m_files)
    {
        // initiate the first read
        auto io_context = new IoContext;

        io_context->size       = pair.size;
        io_context->buffer     = std::make_unique<char[]>(m_chunksize);
        io_context->src_handle = pair.src_handle.get();
        io_context->dst_handle = pair.dst_handle.get();

        ::ZeroMemory(io_context, sizeof(OVERLAPPED));
        
        auto status = ::ReadFile(
            pair.src_handle.get(),
            io_context->buffer.get(),
            m_chunksize,
            nullptr,
            io_context);

        ASSERT(!status && ::GetLastError() == ERROR_IO_PENDING);
    }

    for (auto& t : m_threadpool)
    {
        t.join();
    }
}

void iocp_copy_engine::io_worker()
{
    m_finalize.wait();

    while (m_operation_count.load() > 0)
    {
        auto bytes_xfer = unsigned long{};
        
        auto key = ULONG_PTR{};
        auto ov  = LPOVERLAPPED{};

        // wait for an IO completion 
        if (!wdl::io::get_completion(m_port, &bytes_xfer, &key, &ov, 100))
        {
            if (::GetLastError() != WAIT_TIMEOUT)
            {
                delete ov;
                --m_operation_count;
            }
            continue;
        }

        auto io = static_cast<IoContext*>(ov);

        if (key == (ULONG_PTR)CompletionKey::read)
        {
            auto offset = ULARGE_INTEGER{io->Offset, io->OffsetHigh};
            offset.QuadPart += m_chunksize;

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
            --m_operation_count;
            delete io;
        }
    }
}