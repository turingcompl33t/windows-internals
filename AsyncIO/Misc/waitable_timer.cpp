// waitable_timer.cpp
//
// Demonstration of waitable timer functionality.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 waitable_timer.cpp

#include <windows.h>

#include <cstdio>
#include <string>
#include <stdexcept>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

static void __stdcall timer_callback(
    void* arg,
    unsigned long,
    unsigned long
    )
{
    auto count = *reinterpret_cast<unsigned long*>(arg);

    printf("(%u) Timer Signaled!\n", count);
    return;
}

VOID do_waitable_timer_with_wait(HANDLE timer_handle, LONG period)
{
    auto due_time = LARGE_INTEGER{};

    // due_time argument for the timer is specified 
    // in units of 100ns while the period is specified in ms;
    // here the due_time is set equal to the period
    // negative value for the due_time denotes relative time
    // while a positive value denotes absolute time
    due_time.QuadPart = -(long long)period * 10000;

    if (!::SetWaitableTimer(
        timer_handle,
        &due_time,
        period,
        nullptr,
        nullptr,
        FALSE
        )
    )
    {
        printf("[-] Failed to activate waitable timer\n");
        printf("[-] GLE: %u\n", ::GetLastError());
        return;
    }
    
    for (auto niters = 0u; niters < 10; niters++)
    {
        ::WaitForSingleObject(timer_handle, INFINITE);
        printf("(%u) Timer Signaled!\n", niters);
    }

    ::CancelWaitableTimer(timer_handle);
}

VOID do_waitable_timer_with_callback(HANDLE timer_handle, LONG period)
{
    auto count = unsigned long{};
    auto due_time = LARGE_INTEGER{};

    // due_time argument for the timer is specified 
    // in units of 100ns while the period is specified in ms;
    // here the due_time is set equal to the period
    // negative value for the due_time denotes relative time
    // while a positive value denotes absolute time
    due_time.QuadPart = -(long long)period * 10000;

    if (!::SetWaitableTimer(
        timer_handle,
        &due_time,
        period,
        timer_callback,
        &count,
        FALSE
    ))
    {
        printf("[-] Failed to activate waitable timer\n");
        printf("[-] GLE: %u\n", ::GetLastError());
        return;    
    }

    while (count < 10)
    {
        // enter an alertable wait state to allow timer callback to fire
        ::SleepEx(INFINITE, TRUE);
        count++;
    }

    ::CancelWaitableTimer(timer_handle);
}


int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("[-] Error: invalid arguments\n");
        printf("[-] Usage: %s [WAIT STYLE]\n", argv[0]);
        printf("\t0 -> Wait on Timer Handle\n");
        printf("\t1 -> Callback Routine\n");
        return STATUS_FAILURE_I;
    }

    auto timer_handle = HANDLE{};
    auto wait_style = unsigned long{};

    try
    {
        wait_style = std::stoul(argv[1]);
    }
    catch (const std::exception& e)
    {
        printf("[-] Failed to parse arguments\n");
        printf("[-] %s\n", e.what());
        return STATUS_FAILURE_I;
    }

    // validate the wait style
    if (wait_style > 1)
    {
        printf("[-] Unrecognized wait style specified\n");
        return STATUS_FAILURE_I;
    }
   
    // create the waitable timer object
    timer_handle = ::CreateWaitableTimerW(
        nullptr,
        FALSE,
        nullptr
    );

    if (NULL == timer_handle)
    {
        printf("[-] Failed to create waitable timer\n");
        printf("[-] GLE: %u", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    printf("[+] Successfully created waitable timer\n");

    auto period = 1000l;

    switch (wait_style)
    {
    case 0:
        do_waitable_timer_with_wait(timer_handle, period);
        break;
    case 1:
        do_waitable_timer_with_callback(timer_handle, period);
        break;
    default:
        break;
    }

    return STATUS_SUCCESS_I;
}