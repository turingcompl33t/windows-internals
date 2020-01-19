// ConsoleControlParent.cpp
// Demonstration of sending console control signals programatically.
//
// At least, that was the intention...
// It turns out that a process may only send console control signals
// to processes that share the same console as the sending process
// which, to my eye, completely defeats the purpose.
//
// Build
//  cl /EHsc /nologo ConsoleControlParent.cpp

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <cstdio>
#include <thread>
#include <chrono>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr auto EVENT_NAME   = L"1337Event";
constexpr auto PROGRAM_NAME = L"ConsoleControlChild.exe";

INT wmain()
{
    using namespace std::chrono_literals;

    HANDLE hEvent;
    WCHAR cmdline[MAX_PATH];
    PROCESS_INFORMATION processInfo{};
    STARTUPINFO startupInfo { sizeof(STARTUPINFO) };

    hEvent = ::CreateEventW(
        nullptr,
        FALSE,
        FALSE,
        EVENT_NAME
    );
    if (NULL == hEvent)
    {
        printf("[PARENT] Failed to create event object\n");
        printf("[PARENT] GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    printf("[PARENT] Successfully created event object\n");

    wcscpy_s(cmdline, PROGRAM_NAME);

    if (!::CreateProcessW(
        nullptr,
        cmdline,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NEW_CONSOLE |
        CREATE_NEW_PROCESS_GROUP,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo 
    ))
    {
        printf("[PARENT] Failed to start child process\n");
        printf("[PARENT] GLE: %u\n", ::GetLastError());
        ::CloseHandle(hEvent);
        return STATUS_FAILURE_I;
    }

    printf("[PARENT] Successfully started child process, waiting on event\n");

    // wait for the child to notify us that the handler is registered
    ::WaitForSingleObject(hEvent, INFINITE);

    printf("[PARENT] Event signaled\n");

    // introduce a little delay so we can see child wait in loop
    std::this_thread::sleep_for(3s);

    printf("[PARENT] Sending console control signal\n");

    // generate the console control event
    if (!::GenerateConsoleCtrlEvent(
        CTRL_C_EVENT,
        processInfo.dwProcessId
    ))
    {
        printf("[PARENT] Console control signal generation failed\n");
        printf("[PARENT] GLE: %u\n", ::GetLastError());
    }

    // wait for the child to exit
    ::WaitForSingleObject(processInfo.hProcess, INFINITE);
    
    printf("[PARENT] Child terminated; exiting process\n");

    ::CloseHandle(hEvent);
    ::CloseHandle(processInfo.hProcess);
    ::CloseHandle(processInfo.hThread);

    return STATUS_SUCCESS_I;
}