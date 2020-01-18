// CrtThread.cpp
// "Hello World" CRT thread application.
//
// Build:
//  cl /EHsc /nologo CrtThread.cpp

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <process.h>
#include <tchar.h>
#include <stdio.h>

constexpr auto NTHREADS = 5;

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

struct ThreadArg
{
    DWORD argument;
};

unsigned __stdcall ThreadProc(LPVOID parameter);
VOID LogInfo(LPCTSTR msg);

INT _tmain()
{
    LogInfo(_T("Hello CRT Threads"));

    HANDLE hThread;
    HANDLE Handles[NTHREADS];
    LPVOID Arguments[NTHREADS];

    for (UINT i = 0; i < NTHREADS; ++i)
    {
        Arguments[i] = ::HeapAlloc(
            ::GetProcessHeap(), 
            HEAP_ZERO_MEMORY, 
            sizeof(ThreadArg)
            );

        static_cast<ThreadArg*>(Arguments[i])->argument = i;
    }

    for (UINT i = 0; i < NTHREADS; ++i)
    {
        Handles[i] = (HANDLE) _beginthreadex(
            nullptr,
            0,
            ThreadProc,
            Arguments[i],
            0,
            nullptr
            );
    }

    // wait for all threads to terminate
    ::WaitForMultipleObjects(NTHREADS, Handles, TRUE, INFINITE);

    for (UINT i = 0; i < NTHREADS; ++i)
    {
        ::HeapFree(::GetProcessHeap(), 0, Arguments[i]);
    }

    return STATUS_SUCCESS_I;
}

// thread entry point
unsigned __stdcall ThreadProc(void* pArg)
{
    ThreadArg* pThreadArg = static_cast<ThreadArg*>(pArg);
    DWORD ThreadId = ::GetCurrentThreadId();

    _tprintf(_T("[+] (%-5u) Got Argument: %u\n"), ThreadId, pThreadArg->argument);

    return STATUS_SUCCESS_I;
}

VOID LogInfo(LPCTSTR msg)
{
    _tprintf(_T("[+] %s\n"), msg);
}

