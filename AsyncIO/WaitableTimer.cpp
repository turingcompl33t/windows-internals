// WaitableTimer.cpp
// Demonstration of waitable timer functionality.

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <cstdio>
#include <string>
#include <stdexcept>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

VOID DoWaitableTimerWithWait(HANDLE hTimer, LONG period);
VOID DoWaitableTimerWithCallback(HANDLE hTimer, LONG period);
static VOID APIENTRY TimerCallback(
    LPVOID lpArg,
    DWORD dwTimerLow,
    DWORD dwTimerHigh
);

INT wmain(INT argc, WCHAR *argv[])
{
    if (argc != 2)
    {
        printf("[-] Error: invalid arguments\n");
        printf("[-] Usage: %ws [WAIT STYLE]\n", argv[0]);
        printf("\t0 -> Wait on Timer Handle\n");
        printf("\t1 -> Callback Routine\n");
        return STATUS_FAILURE_I;
    }

    HANDLE hTimer;
    unsigned long waitStyle;

    try
    {
        waitStyle = std::stoul(argv[1]);
    }
    catch (const std::exception& e)
    {
        printf("[-] Failed to parse arguments\n");
        printf("[-] %s\n", e.what());
        return STATUS_FAILURE_I;
    }

    // validate the wait style
    if (waitStyle > 1)
    {
        printf("[-] Unrecognized wait style specified\n");
        return STATUS_FAILURE_I;
    }
   
    // create the waitable timer object
    hTimer = ::CreateWaitableTimerW(
        nullptr,
        FALSE,
        nullptr
    );

    if (NULL == hTimer)
    {
        printf("[-] Failed to create waitable timer\n");
        printf("[-] GLE: %u", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    printf("[+] Successfully created waitable timer\n");

    LONG period = 1000;

    switch (waitStyle)
    {
    case 0:
        DoWaitableTimerWithWait(hTimer, period);
        break;
    case 1:
        DoWaitableTimerWithCallback(hTimer, period);
        break;
    default:
        break;
    }

    return STATUS_SUCCESS_I;
}

VOID DoWaitableTimerWithWait(HANDLE hTimer, LONG period)
{
    LARGE_INTEGER dueTime;

    // dueTime argument for the timer is specified 
    // in units of 100ns while the period is specified in ms;
    // here the dueTime is set equal to the period
    // negative value for the dueTime denotes relative time
    // while a positive value denotes absolute time
    dueTime.QuadPart = -(LONGLONG)period * 10000;

    if (!::SetWaitableTimer(
        hTimer,
        &dueTime,
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
    
    for (ULONG niters = 0; niters < 10; niters++)
    {
        WaitForSingleObject(hTimer, INFINITE);
        printf("(%u) Timer Signaled!\n", niters);
    }

    ::CancelWaitableTimer(hTimer);
}

VOID DoWaitableTimerWithCallback(HANDLE hTimer, LONG period)
{
    DWORD count = 0;
    LARGE_INTEGER dueTime;

    // dueTime argument for the timer is specified 
    // in units of 100ns while the period is specified in ms;
    // here the dueTime is set equal to the period
    // negative value for the dueTime denotes relative time
    // while a positive value denotes absolute time
    dueTime.QuadPart = -(LONGLONG)period * 10000;

    if (!::SetWaitableTimer(
        hTimer,
        &dueTime,
        period,
        TimerCallback,
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

    ::CancelWaitableTimer(hTimer);
}

static VOID APIENTRY TimerCallback(
    LPVOID lpArg,
    DWORD dwTimerLow,
    DWORD dwTimerHigh
)
{
    auto count = *reinterpret_cast<LPDWORD>(lpArg);

    printf("(%u) Timer Signaled!\n", count);
    return;
}
