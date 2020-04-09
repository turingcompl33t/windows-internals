// share_dup_src.cpp
//
// Demonstrates sharing via kernel object duplication.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 share_dup_src.cpp

#include <windows.h>
#include <cstdio>

struct MESSAGE
{
    ULONG HandleValue;
};

INT main(INT argc, PCHAR argv[])
{
    HANDLE hReadPipe;
    HANDLE hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES) };
    sa.bInheritHandle = TRUE;

    WCHAR SinkApp[] = L"ShareDuplicateSink.exe";

    if (!::CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        printf("[SOURCE] Failed to create pipe; GLE: %u\n", GetLastError());
        return 1;
    }

    puts("[SOURCE] Successfully created pipe");

    PROCESS_INFORMATION ProcInfo;
    
    STARTUPINFO StartupInfo = { sizeof(STARTUPINFO) }; 
    StartupInfo.dwFlags     = STARTF_USESTDHANDLES;
    StartupInfo.hStdInput   = hReadPipe;
    StartupInfo.hStdOutput  = ::GetStdHandle(STD_OUTPUT_HANDLE);
    StartupInfo.hStdError   = ::GetStdHandle(STD_ERROR_HANDLE);

    if (!::CreateProcessW(
        nullptr, 
        SinkApp, 
        nullptr, 
        nullptr, 
        TRUE,
        0,
        nullptr,
        nullptr,
        &StartupInfo,
        &ProcInfo
        )
    )
    {
        printf("[SOURCE] Failed to create process; GLE: %u\n", GetLastError());
        return 1;
    }

    puts("[SOURCE] Successfully created sink process");
    ::CloseHandle(ProcInfo.hThread);

    // create an event handle to duplicate
    // (creating an unnamed event here precludes the possibility
    //  of sharing via kernel object name)
    HANDLE hEvent = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (NULL == hEvent)
    {
        printf("[SOURCE] Failed to create event object; GLE: %u\n", ::GetLastError());
        return 1;
    }

    printf(
        "[SOURCE] Successfully created event object; handle value %u\n", 
        ::HandleToULong(hEvent)
        );

    // duplicate the event handle to sink process
    HANDLE hEventSink;
    if (!::DuplicateHandle(
        GetCurrentProcess(), 
        hEvent, 
        ProcInfo.hProcess,
        &hEventSink,
        0,
        FALSE,
        DUPLICATE_SAME_ACCESS
        )
    )
    {
        printf(
            "[SOURCE] Failed to duplicate handle to sink process; GLE: %u\n", 
            ::GetLastError()
            );
        return 1;
    }

    printf("[SOURCE] Successfully duplicated handle to sink process\n");

    // communicate the handle value to sink process
    MESSAGE m;
    m.HandleValue = ::HandleToULong(hEventSink);

    if (!::WriteFile(
        hWritePipe, 
        &m, 
        sizeof(MESSAGE),
        nullptr,
        nullptr
        )
    )
    {
        printf(
            "[SOURCE] Failed to communicate duped handle value to sink process; GLE: %u\n", 
            GetLastError()
            );
        return 1;
    }

    puts("[SOURCE] Successfully communicated duped handle value to sink process");
    puts("[SOURCE] Waiting for sink process to signal event");

    ::WaitForSingleObject(hEvent, INFINITE);

    puts("[SOURCE] Sink process signaled event; exiting");

    return 0;
}