// share_named_parent.cpp
//
// Demonstrates kernel object handle sharing via named object.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 share_named_parent.cpp

#include <windows.h>
#include <string>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto CMDLINE_BUFSIZE = 128;

constexpr const auto EVENT_NAME = L"SharedNameEvent";

int main()
{
    auto process_info = PROCESS_INFORMATION{};
    auto startup_info = STARTUPINFOW{ sizeof(STARTUPINFOW) };

    auto sa = SECURITY_ATTRIBUTES{ sizeof(SECURITY_ATTRIBUTES) };
    sa.bInheritHandle = FALSE;

    // create a manual reset event
    auto event = ::CreateEventW(&sa, TRUE, FALSE, EVENT_NAME);
    if (NULL == event)
    {
        printf("[PARENT] Failed to create event; GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    wchar_t cmdline[CMDLINE_BUFSIZE];
    swprintf_s(cmdline, L"share_named_child.exe");

    if (!::CreateProcessW(
        nullptr,
        cmdline,
        nullptr,
        nullptr,
        TRUE,
        0,
        nullptr,
        nullptr,
        &startup_info,
        &process_info))
    {
        printf("[PARENT] Failed to start child process; GLE: %u\n", ::GetLastError());
        ::CloseHandle(event);
        return STATUS_FAILURE_I;
    }

    printf("[PARENT] Successfully started child process; signaling event and waiting for child exit\n");

    ::SetEvent(event);

    ::WaitForSingleObject(process_info.hProcess, INFINITE);

    puts("[PARENT] Child process completed; process exiting");

    ::CloseHandle(process_info.hProcess);
    ::CloseHandle(process_info.hThread);
    ::CloseHandle(event);

    return STATUS_SUCCESS_I;
}