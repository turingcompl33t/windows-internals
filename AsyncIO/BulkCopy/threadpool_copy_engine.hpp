// threadpool_copy_engine.cpp
//
// Bulk filesystem copy engine utilizing Windows threadpool.

#include <windows.h>

#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <memory>

#include <wdl/debug.hpp>
#include <wdl/handle.hpp>
#include <wdl/threadpool/pool.hpp>
#include <wdl/synchronization/event.hpp>

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

class threadpool_copy_engine
{
    using invalid_handle = wdl::handle::invalid_handle;
    using event          = wdl::synchronization::event;
    using event_type     = wdl::synchronization::event_type;

    unsigned long           m_threadcount;
    unsigned long           m_chunksize;

    wdl::threadpool::pool    m_pool;
    std::vector<FilePair>    m_files;

    std::atomic_uint64_t     m_operation_count{ 0 };

    event m_complete{ event_type::manual_reset };

public:
    threadpool_copy_engine(
        unsigned long threadcount = std::thread::hardware_concurrency(),
        unsigned long chunksize   = 1 << 16)
        : m_threadcount{ threadcount }
        , m_chunksize{ chunksize }
    {
        m_pool.set_max_threadcount(threadcount);
    }

    threadpool_copy_engine(threadpool_copy_engine const&)            = delete;
    threadpool_copy_engine& operator=(threadpool_copy_engine const&) = delete;

    threadpool_copy_engine(threadpool_copy_engine&&)            = delete;
    threadpool_copy_engine& operator=(threadpool_copy_engine&&) = delete;

    ~threadpool_copy_engine() = default;

    void add_file(std::string const& src_file, std::string const& dst_file);

    void finalize();
    void start();

private:
    static void __stdcall read_completion_handler(
        PTP_CALLBACK_INSTANCE,
        void*,
        void*, 
        unsigned long, 
        ULONG_PTR, 
        PTP_IO);

    static void __stdcall write_completion_handler(
        PTP_CALLBACK_INSTANCE,
        void*,
        void*, 
        unsigned long, 
        ULONG_PTR, 
        PTP_IO);
};

void threadpool_copy_engine::add_file(
    std::string const& src_file,
    std::string const& dst_file
    )
{
    m_files.emplace_back(src_file, dst_file);
}

void threadpool_copy_engine::finalize()
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
        
        // create IO objects for src and dst
        auto src_io = wdl::handle::io_handle { 
            wdl::threadpool::create_io(m_pool, src.get(), read_completion_handler, this) };
        auto dst_io = wdl::handle::io_handle {
            wdl::threadpool::create_io(m_pool, dst.get(), write_completion_handler, this) };

        pair.size       = size.QuadPart;
        pair.src_handle = std::move(src);
        pair.dst_handle = std::move(dst);
        pair.src_io     = std::move(src_io);
        pair.dst_io     = std::move(dst_io);
        
        // track the total number of operations required for all operations
        m_operation_count.fetch_add((size.QuadPart + m_chunksize - 1) / m_chunksize);
    }
}

void threadpool_copy_engine::start()
{
    for (auto& pair : m_files)
    {
        // initiate the first read
        auto io_context = new IoContext;

        io_context->size       = pair.size;
        io_context->buffer     = std::make_unique<char[]>(m_chunksize);
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
            m_chunksize,
            nullptr,
            io_context);

        ASSERT(!status && ::GetLastError() == ERROR_IO_PENDING);
    }  

    m_complete.wait();
}

void __stdcall threadpool_copy_engine::read_completion_handler(
    PTP_CALLBACK_INSTANCE,
    void*         ctx,
    void*         ov, 
    unsigned long io_result, 
    ULONG_PTR     bytes_xfer, 
    PTP_IO
    )
{    
    ASSERT(io_result == ERROR_SUCCESS);

    // read operation has completed
    auto io = static_cast<IoContext*>(ov);
    auto engine = static_cast<threadpool_copy_engine*>(ctx);

    auto offset = ULARGE_INTEGER{io->Offset, io->OffsetHigh};
    offset.QuadPart += engine->m_chunksize;

    // determine if read operations remain to be completed
    if (offset.QuadPart < io->size)
    {
        auto new_io = new IoContext;

        new_io->size       = io->size;
        new_io->buffer     = std::make_unique<char[]>(engine->m_chunksize);
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
            engine->m_chunksize,
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

void __stdcall threadpool_copy_engine::write_completion_handler(
    PTP_CALLBACK_INSTANCE,
    void*         ctx,
    void*         ov, 
    unsigned long io_result, 
    ULONG_PTR, 
    PTP_IO
    )
{
    ASSERT(io_result == ERROR_SUCCESS);

    auto io_context = static_cast<IoContext*>(ov);
    auto engine = static_cast<threadpool_copy_engine*>(ctx);

    delete io_context;
    
    auto remaining = --engine->m_operation_count;
    if (remaining == 0)
    {
        // all operations completed
        engine->m_complete.set();
    }
} 
