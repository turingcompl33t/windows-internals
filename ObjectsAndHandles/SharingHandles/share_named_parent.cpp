// share_named_parent.cpp
//
// Demonstrates kernel object handle sharing by name.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 share_named_parent.cpp

#include <windows.h>

#include <string>

constexpr auto EventName = L"SharedNameEvent";

int main()
{
    HANDLE hEvent;
    HANDLE hProcess;
    WCHAR cmdline[128];

    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo = { sizeof(STARTUPINFO) };

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES) };
    sa.bInheritHandle = FALSE;

    // create a manual reset event
    hEvent = ::CreateEventW(&sa, TRUE, FALSE, EventName);
    if (NULL == hEvent)
    {
        printf("[PARENT] Failed to create event; GLE: %u\n", ::GetLastError());
        return 1;
    }

    swprintf(cmdline, L"ShareNamedChild.exe");

    if (!::CreateProcessW(
        nullptr,
        cmdline,
        nullptr,
        nullptr,
        TRUE,
        0,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo
        )
    )
    {
        printf("[PARENT] Failed to start child process; GLE: %u\n", ::GetLastError());
        ::CloseHandle(hEvent);
        return 1;
    }

    printf("[PARENT] Successfully started child process; signaling event and waiting for child exit\n");

    ::SetEvent(hEvent);

    ::WaitForSingleObject(processInfo.hProcess, INFINITE);

    puts("[PARENT] Child process completed; process exiting");

    ::CloseHandle(processInfo.hProcess);
    ::CloseHandle(processInfo.hThread);
    ::CloseHandle(hEvent);

    return 0;
}