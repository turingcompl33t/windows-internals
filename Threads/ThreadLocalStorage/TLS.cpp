// TLS.cpp
// Demo of thread-local storage utilization.
//
// NOTE: this program makes egregious misuse of 
// Windows synchronization objects and error 
// handling facilities; forgive me.

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

constexpr UINT NINDICES = 2;
constexpr UINT NTHREADS = 2;

DWORD WINAPI DoTls(LPVOID);

struct ThreadArgs
{
    CRITICAL_SECTION  cs;
    LONG volatile     lUpCount;
};

INT _tmain(VOID)
{
    _tprintf(TEXT("Hello TLS!\n"));

    DWORD TlsIndices[NINDICES];
    HANDLE ThreadHandles[NTHREADS];

    for (UINT i = 0; i < NINDICES; ++i)
    {
        TlsIndices[i] = TlsAlloc();
        if (TLS_OUT_OF_INDEXES == TlsIndices[i])
        {
            _tprintf(TEXT("Failed to allocate TLS index %u; out of indices!\n"), i);
        }
        _tprintf(TEXT("Successfully allocated TLS index %u\n"), i);
    }

    ThreadArgs args;
    InitializeCriticalSection(&args.cs);
    args.lUpCount = NTHREADS;

    for (UINT i = 0; i < NTHREADS; ++i)
    {
        ThreadHandles[i] = CreateThread(NULL, 0, SetTls, &args, 0, NULL);
    }

    // wait for all the threads to set TLS values
    WaitForMultipleObjects(NTHREADS, ThreadHandles, TRUE, INFINITE);

    for (UINT i = 0; i < NTHREADS; ++i)
    {
        CloseHandle(ThreadHandles[i]);
    }

    return 0;
}

DWORD WINAPI DoTls(LPVOID lpArg)
{
    ThreadArgs* args = static_cast<ThreadArgs*>(lpArg);
    
    DWORD dwThreadId = GetCurrentThreadId();

    __try
    {
        EnterCriticalSection(&args->cs);

        for (UINT i = 0; i < NINDICES; ++i)
        {
            PDWORD ptr = static_cast<PDWORD>(
                HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE, sizeof(DWORD))
                );
            
            *ptr = dwThreadId + i;
            
            TlsSetValue(i, ptr);

            _tprintf(TEXT("Thread %u set TLS index %u to %u\n"), dwThreadId, i, *ptr);
        }

    }
    __finally
    {
        LeaveCriticalSection(&args->cs);
    }

    InterlockedDecrement(&args->lUpCount);

    // poll the up count
    while (0 != args->lUpCount)
    {
        Sleep(500);
    }

    __try
    {
        EnterCriticalSection(&args->cs);
        for (UINT i = 0; i < NINDICES; ++i)
        {   
            PDWORD ptr = static_cast<PDWORD>(TlsGetValue(i));
            _tprintf(TEXT("Thread %u read value %u from TLS index %u\n"), dwThreadId, *ptr, i);

            HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, ptr);
        }
    }
    __finally
    {   
        LeaveCriticalSection(&args->cs);
    }

    return 0;
}