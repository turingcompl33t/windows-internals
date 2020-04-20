// crt_thread.cpp
// "Hello World" CRT thread application.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 crt_thread.cpp

#include <windows.h>
#include <process.h>
#include <stdio.h>

constexpr const auto N_THREADS = 5;

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

struct thread_arg_t
{
    unsigned long n;
};

// thread entry point
unsigned __stdcall thread_proc(void* param)
{
    thread_arg_t* arg = static_cast<thread_arg_t*>(param);
    auto thread_id = ::GetCurrentThreadId();

    printf("[+] (%-5u) Got Argument: %u\n", thread_id, arg->n);

    return STATUS_SUCCESS_I;
}

int main()
{
    LPVOID arguments[N_THREADS];
    for (auto i = 0u; i < N_THREADS; ++i)
    {
        arguments[i] = ::HeapAlloc(
            ::GetProcessHeap(), 
            HEAP_ZERO_MEMORY, 
            sizeof(thread_arg_t));

        static_cast<thread_arg_t*>(arguments[i])->n = i;
    }

    HANDLE threads[N_THREADS];
    for (auto i = 0u; i < N_THREADS; ++i)
    {
        threads[i] = (HANDLE) _beginthreadex(
            nullptr,
            0,
            thread_proc,
            arguments[i],
            0,
            nullptr);
    }

    // wait for all threads to terminate
    ::WaitForMultipleObjects(N_THREADS, threads, TRUE, INFINITE);

    for (auto i = 0u; i < N_THREADS; ++i)
    {
        ::HeapFree(::GetProcessHeap(), 0, arguments[i]);
    }

    return STATUS_SUCCESS_I;
}


