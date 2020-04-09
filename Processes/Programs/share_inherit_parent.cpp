// share_inherit_parent.cpp
//
// Demonstrates sharing of kernel object handle via inheritance.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 share_inherit_parent.cpp

#include <windows.h>
#include <cstdio>

int main(int argc, char* argv[])
{
    HANDLE hEvent;
    HANDLE hProcess;
    WCHAR cmdline[128];

    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo = { sizeof(STARTUPINFO) };

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES) };
    sa.bInheritHandle = TRUE;

    hEvent = ::CreateEventW(&sa, TRUE, FALSE, nullptr);
    if (NULL == hEvent)
    {
        printf("[PARENT] Failed to create event; GLE: %u\n", ::GetLastError());
        return 1;
    }

    swprintf(cmdline, L"ShareInheritChild.exe %u", ::HandleToULong(hEvent));

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

    // event is manual reset
    ::SetEvent(hEvent);

    ::WaitForSingleObject(processInfo.hProcess, INFINITE);

    printf("[PARENT] Child process exited; process exiting\n");

    ::CloseHandle(processInfo.hThread);
    ::CloseHandle(processInfo.hProcess);
    ::CloseHandle(hEvent);

    return 0;
}