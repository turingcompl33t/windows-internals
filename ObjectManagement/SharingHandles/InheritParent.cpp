// InheritParent.cpp
// Demonstration of sharing handles via inheritance.

#define UNICODE
#define _UNICODE

#include <windows.h>

#include <cstdio>

INT main(INT argc, PCHAR argv[])
{
    HANDLE hEvent;
    HANDLE hProcess;
    WCHAR cmdline[128];

    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo = { sizeof(STARTUPINFO) };

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES) };
    sa.bInheritHandle = TRUE;

    hEvent = ::CreateEvent(&sa, TRUE, FALSE, nullptr);
    if (NULL == hEvent)
    {
        printf("[PARENT] Failed to create event; GLE: %u\n", GetLastError());
        return 1;
    }

    swprintf(cmdline, L"InheritChild.exe %u", ::HandleToUlong(hEvent));

    if (!::CreateProcess(
        nullptr,
        cmdline,
        nullptr,
        nullptr,
        TRUE,
        0,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo)
    )
    {
        printf("[PARENT] Failed to start child process; GLE: %u\n", GetLastError());
        CloseHandle(hEvent);
        return 1;
    }

    printf("[PARENT] Successfully started child process; signaling event and waiting for child exit\n");

    // event is manual reset
    SetEvent(hEvent);

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    printf("[PARENT] Child process exited; cleaning up shop\n");

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    CloseHandle(hEvent);

    return 0;
}