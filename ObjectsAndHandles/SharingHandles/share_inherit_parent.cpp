// share_inherit_parent.cpp
//
// Demonstrates sharing of kernel object handle via inheritance.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 share_inherit_parent.cpp

#include <windows.h>
#include <cstdio>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto CMDLINE_BUFSIZE = 128;

int main()
{
    auto process_info = PROCESS_INFORMATION{};
    auto startup_info = STARTUPINFOW{ sizeof(STARTUPINFOW) };

    auto sa = SECURITY_ATTRIBUTES{ sizeof(SECURITY_ATTRIBUTES) };
    sa.bInheritHandle = TRUE;

    auto event = ::CreateEventW(&sa, TRUE, FALSE, nullptr);
    if (NULL == event)
    {
        printf("[PARENT] Failed to create event; GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    wchar_t cmdline[CMDLINE_BUFSIZE];
    swprintf_s(cmdline, L"share_inherit_child.exe %u", ::HandleToULong(event));

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

    printf("[PARENT] Child process exited; process exiting\n");

    ::CloseHandle(process_info.hThread);
    ::CloseHandle(process_info.hProcess);
    ::CloseHandle(event);

    return STATUS_SUCCESS_I;
}