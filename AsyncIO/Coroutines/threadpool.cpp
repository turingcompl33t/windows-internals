// threadpool.cpp
//
// A simple pool_awaiter that allows us to resume execution
// on a Windows threadpool context.
//
// Build
//  cl /EHsc /nologo /std:c++latest /await /W4 threadpool.cpp

#include <windows.h>
#include <winrt/Windows.Foundation.h>

#include <cstdio>
#include <experimental/coroutine>

#pragma comment(lib, "windowsapp")

class win32_error
{
    unsigned long m_code;
public:
    explicit win32_error(unsigned long const code = ::GetLastError())
        : m_code{ code }
    {}

    unsigned long code() const noexcept
    {
        return m_code;
    }
};

struct pool_awaiter 
    : std::experimental::suspend_always
{
    // override the await_suspend() provided by suspend_always
    void await_suspend(std::experimental::coroutine_handle<> handle)
    {
        auto const result = ::TrySubmitThreadpoolCallback(
            pool_awaiter_callback, 
            handle.address(), 
            nullptr);
        if (!result) throw win32_error{};
    }

private:
    static void __stdcall pool_awaiter_callback(PTP_CALLBACK_INSTANCE, void* ctx)
    {
        // invoke operator() on the coroutine handle to resume the coroutine
        // the coroutine is resumed on the threadpool thread
        std::experimental::coroutine_handle<>::from_address(ctx)();
    }
};

// not really necessary, but convenient
// (pool_awaiter ctor can also be [[nodiscard]])
[[nodiscard]]
auto resume_threadpool()
{
    return pool_awaiter{};
}

winrt::fire_and_forget do_work(HANDLE event)
{
    // executed in calling thread context
    printf("Setup: %u\n", ::GetCurrentThreadId());

    co_await resume_threadpool();

    // executed in context of threadpool thead
    printf("Work: %u\n", ::GetCurrentThreadId());
    ::SetEvent(event);
}

int main()
{
    auto const event = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    
    do_work(event);

    ::WaitForSingleObject(event, INFINITE);
    
    return 0;
}