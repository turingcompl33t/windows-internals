// ConsoleControlChild.cpp
// Demonstration of sending console control signals programatically.
//
// At least, that was the intention...
// It turns out that a process may only send console control signals
// to processes that share the same console as the sending process
// which, to my eye, completely defeats the purpose.
//
// Build:
//  cl /EHsc /nologo ConsoleControlChild.cpp


#define UNICODE
#define _UNICODE

#include <windows.h>
#include <cstdio>
#include <thread>
#include <chrono>
#include <atomic>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr auto EVENT_NAME   = L"1337Event";

std::atomic_bool flag{};

BOOL HandlerCallback(DWORD ControlCode);

INT wmain()
{
    using namespace std::chrono_literals;

    HANDLE hEvent;

    printf("[CHILD] Process started\n");

    hEvent = ::OpenEventW(EVENT_ALL_ACCESS, FALSE, EVENT_NAME);
    if (NULL == hEvent)
    {
        printf("[CHILD] Failed to acquire handle to event\n");
        printf("[CHILD] GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    printf("[CHILD] Successfully acquired handle to event; registering handler\n");

    ::SetConsoleCtrlHandler(HandlerCallback, TRUE);

    printf("[CHILD] Handler set; ready to go\n");

    ::SetEvent(hEvent);

    while (!flag)
    {   
        printf("[CHILD] Waiting for signal\n");
        std::this_thread::sleep_for(1s);
    }

    printf("[CHILD] Process exiting\n");
}

BOOL HandlerCallback(DWORD ControlCode)
{
    printf("[CHILD] Console control handler invoked\n");

    switch (ControlCode)
    {
    case CTRL_C_EVENT:
        printf("[CHILD] CTRL_C_EVENT received\n");
        break;
    case CTRL_CLOSE_EVENT:
        printf("[CHILD] CTRL_CLOSE_EVENT received\n");
        break;
    case CTRL_BREAK_EVENT:
        printf("[CHILD] CTRL_BREAK_EVENT received\n");
        break;
    case CTRL_LOGOFF_EVENT:
        printf("[CHILD] CTRL_LOGOFF_EVENT received\n");
        break;
    case CTRL_SHUTDOWN_EVENT:
        printf("[CHILD] CTRL_SHUTDOWN_EVENT received\n");
        break;
    }

    flag = true;

    return TRUE;
}