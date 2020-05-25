// winrt_private_pool.cpp
//
// Build
//  cl /EHsc /nologo /std:c++latest /await winrt_private_pool.cpp

#include <windows.h>
#include <winrt/Windows.Foundation.h>

#pragma comment(lib, "windowsapp")

class private_pool
{
    PTP_POOL            m_pool;
    TP_CALLBACK_ENVIRON m_environment;

public:
    private_pool() 
        : m_pool{ ::CreateThreadpool(nullptr) }
    {
        ::InitializeThreadpoolEnvironment(&m_environment);
        ::SetThreadpoolCallbackPool(&m_environment, m_pool);
    }

    ~private_pool() noexcept
    {
        ::DestroyThreadpoolEnvironment(&m_environment);
        ::CloseThreadpool(m_pool);
    }

    void thread_limits(uint32_t const high, uint32_t const low)
    {
        ::SetThreadpoolThreadMaximum(m_pool, high);
        ::SetThreadpoolThreadMinimum(m_pool, low);
    }

    bool await_ready() const noexcept
    {
        return false;
    }

    void await_resume() const noexcept
    {}

    void await_suspend(std::experimental::coroutine_handle<> handle)
    {
        if (!::TrySubmitThreadpoolCallback(callback, handle.address(), &m_environment))
        {
            winrt::throw_last_error();
        }
    }

private:
    static void WINRT_CALL callback(
        PTP_CALLBACK_INSTANCE, 
        void* context
        ) noexcept
    {
        std::experimental::coroutine_handle<>::from_address(context)();
    }
};

class async_queue
{
    private_pool m_pool;

public:
    async_queue()
    {
        // ensures that all work on this queue is performed by a single threadpool thread
        m_pool.thread_limits(1, 1);
    }

    winrt::Windows::Foundation::IAsyncAction 
    async(winrt::delegate<> callback)
    {
        // this uses the default process-wide thread pool
        // co_await resume_background();

        // this uses the private m_pool thread pool;         
        // the pool must outlive the coroutine
        co_await m_pool;

        callback();
    }
};

int main()
{
    winrt::init_apartment();

    auto queue   = async_queue{};
    auto results = std::vector<winrt::Windows::Foundation::IAsyncAction>{};

    for (auto item = uint32_t{}; item < 20; ++item)
    {
        results.push_back(queue.async([=]
            {
                ::printf("Item=%d Thread=%u\n", item, ::GetCurrentThreadId());
            }));
    }

    ::printf("Primary thread=%u\n", ::GetCurrentThreadId());

    for (auto&& async : results)
    {
        async.get();
    }

    ::puts("Complete");
}