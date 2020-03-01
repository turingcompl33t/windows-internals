// Win32Thread.cpp
// "Hello World" Win32 thread application.
//
// Build:
//  cl /EHsc /nologo Win32Thread.cpp

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

constexpr auto NTHREADS = 5;

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

struct ThreadArg
{
    DWORD argument;
};

DWORD WINAPI ThreadProc(LPVOID parameter);
VOID LogInfo(LPCTSTR msg);

INT _tmain()
{
    LogInfo(_T("Hello Win32 Threads"));

    HANDLE hThread;
    HANDLE Handles[NTHREADS];
    LPVOID Arguments[NTHREADS];

    for (UINT i = 0; i < NTHREADS; ++i)
    {
        Arguments[i] = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ThreadArg));
        ThreadArg* arg = static_cast<ThreadArg*>(Arguments[i]);
        arg->argument = i;
    }

    for (UINT i = 0; i < NTHREADS; ++i)
    {
        hThread = ::CreateThread(NULL, 0, ThreadProc, Arguments[i], 0, NULL);
        Handles[i] = hThread;
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
DWORD WINAPI ThreadProc(LPVOID parameter)
{
    ThreadArg* arg = static_cast<ThreadArg*>(parameter);
    DWORD ThreadId = ::GetCurrentThreadId();

    _tprintf(_T("[+] (%-5u) Got Argument: %u\n"), ThreadId, arg->argument);

    return STATUS_SUCCESS_I;
}

VOID LogInfo(LPCTSTR msg)
{
    _tprintf(_T("[+] %s\n"), msg);
}

