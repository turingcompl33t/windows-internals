// win32_thread.cpp
// "Hello World" Win32 thread application.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 win32_thread.cpp

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

constexpr const auto NTHREADS = 5;

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

struct thread_arg_t
{
    unsigned long n;
};

// thread entry point
unsigned long WINAPI ThreadProc(void* param)
{
    thread_arg_t* arg = static_cast<thread_arg_t*>(param);
    auto thread_id = ::GetCurrentThreadId();

    printf("[+] (%-5u) Got Argument: %u\n", thread_id, arg->n);

    return STATUS_SUCCESS_I;
}

int main()
{
    HANDLE hThread;
    HANDLE Handles[NTHREADS];
    LPVOID Arguments[NTHREADS];

    for (auto i = 0u; i < NTHREADS; ++i)
    {
        Arguments[i] = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(thread_arg_t));
        thread_arg_t* arg = static_cast<thread_arg_t*>(Arguments[i]);
        arg->n = i;
    }

    for (auto i = 0u; i < NTHREADS; ++i)
    {
        hThread = ::CreateThread(NULL, 0, ThreadProc, Arguments[i], 0, NULL);
        Handles[i] = hThread;
    }

    // wait for all threads to terminate
    ::WaitForMultipleObjects(NTHREADS, Handles, TRUE, INFINITE);

    for (auto i = 0u; i < NTHREADS; ++i)
    {
        ::HeapFree(::GetProcessHeap(), 0, Arguments[i]);
    }

    return STATUS_SUCCESS_I;
}