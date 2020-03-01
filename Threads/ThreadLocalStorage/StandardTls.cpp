// StandardTls.cpp
// Demonstration of C++11 support for thread local storage objects.
//
// Build:
//  cl /EHsc /nologo /std:c++17 StandardTls.cpp

#include <windows.h>
#include <thread>
#include <future>
#include <cstdio>
#include <chrono>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr unsigned T1_INCREMENT = 5;
constexpr unsigned T2_INCREMENT = 10;

thread_local unsigned long g_value = 1;

void thread_fn(int increment);
unsigned long thread_id();

int wmain()
{
    auto f1 = std::async(std::launch::async, thread_fn, T1_INCREMENT);
    auto f2 = std::async(std::launch::async, thread_fn, T2_INCREMENT);

    f1.get();
    f2.get();

    return STATUS_SUCCESS_I;
}

void thread_fn(int increment)
{
    using namespace std::chrono_literals;

    auto tid = thread_id();

    printf("[%u] I increment by %d\n", tid, increment);

    for (;;)
    {
        printf("[%u] g_value = %u\n", tid, g_value);
        g_value += increment;

        std::this_thread::sleep_for(2s);
    }

    return;
}

unsigned long thread_id()
{
    return ::GetCurrentThreadId();
}