// share_dup_src.cpp
//
// Demonstrates sharing via kernel object duplication.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 share_dup_src.cpp

#include <windows.h>
#include <cstdio>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto CMDLINE_BUFSIZE = 128;

struct MESSAGE
{
    unsigned long handle_num;
};

int main()
{
    auto read_pipe  = HANDLE{};
    auto write_pipe = HANDLE{};
    
    auto sa = SECURITY_ATTRIBUTES{ sizeof(SECURITY_ATTRIBUTES) };
    sa.bInheritHandle = TRUE;

    if (!::CreatePipe(&read_pipe, &write_pipe, &sa, 0))
    {
        printf("[SOURCE] Failed to create pipe; GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    puts("[SOURCE] Successfully created pipe");

    auto process_info = PROCESS_INFORMATION{};
    
    auto startup_info = STARTUPINFOW{ sizeof(STARTUPINFOW) }; 
    startup_info.dwFlags     = STARTF_USESTDHANDLES;
    startup_info.hStdInput   = read_pipe;
    startup_info.hStdOutput  = ::GetStdHandle(STD_OUTPUT_HANDLE);
    startup_info.hStdError   = ::GetStdHandle(STD_ERROR_HANDLE);

    wchar_t cmdline[CMDLINE_BUFSIZE];
    wcscpy_s(cmdline, L"share_dup_sink.exe");

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
        printf("[SOURCE] Failed to create process; GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    puts("[SOURCE] Successfully created sink process");
    ::CloseHandle(process_info.hThread);
    ::CloseHandle(read_pipe);

    // create an event handle to duplicate
    // (creating an unnamed event here precludes the possibility
    //  of sharing via kernel object name)
    auto event = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (NULL == event)
    {
        printf(
            "[SOURCE] Failed to create event object; GLE: %u\n", 
            ::GetLastError());
        return STATUS_FAILURE_I;
    }

    printf(
        "[SOURCE] Successfully created event object; handle value %u\n", 
        ::HandleToULong(event));

    // duplicate the event handle to sink process
    auto event_dup = HANDLE{};
    if (!::DuplicateHandle(
        GetCurrentProcess(), 
        event, 
        process_info.hProcess,
        &event_dup,
        0,
        FALSE,
        DUPLICATE_SAME_ACCESS))
    {
        printf(
            "[SOURCE] Failed to duplicate handle to sink process; GLE: %u\n", 
            ::GetLastError());
        return STATUS_FAILURE_I;
    }

    printf(
        "[SOURCE] Successfully duplicated handle to sink process; handle value %u\n", 
        ::HandleToULong(event_dup));

    // communicate the handle value to sink process
    auto m = MESSAGE{};
    m.handle_num = ::HandleToULong(event_dup);

    if (!::WriteFile(
        write_pipe, 
        &m, 
        sizeof(MESSAGE),
        nullptr,
        nullptr))
    {
        printf(
            "[SOURCE] Failed to communicate duped handle value to sink process; GLE: %u\n", 
            ::GetLastError());
        return STATUS_FAILURE_I;
    }

    puts("[SOURCE] Successfully communicated duped handle value to sink process");
    puts("[SOURCE] Waiting for sink process to signal event");

    ::WaitForSingleObject(event, INFINITE);

    puts("[SOURCE] Sink process signaled event; exiting");

    return STATUS_SUCCESS_I;
}