// ConsoleControl.cpp
// Demonstration of a program that registers a console control handler.

#include <windows.h>
#include <cstdio>
#include <thread>
#include <chrono>
#include <atomic>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

std::atomic_bool flag{};

BOOL HandlerCallback(DWORD ControlCode);

INT main()
{
    using namespace std::chrono_literals;

    if (!::SetConsoleCtrlHandler(
        HandlerCallback,
        TRUE
    ))
    {
        printf("[-] Failed to register console control handler\n");
        printf("[-] GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    printf("[+] Successfully registered console control handler\n");
    printf("[+] Sitting in loop until interrupted\n");

    while  (!flag)
    {
        printf("[+] Looping\n");
        std::this_thread::sleep_for(1s);
    }

    printf("[+] Loop broken, exiting program\n");

    return STATUS_SUCCESS_I;
}

BOOL HandlerCallback(DWORD ControlCode)
{
    printf("[+] Console control handler invoked\n");

    switch (ControlCode)
    {
    case CTRL_C_EVENT:
        printf("[+] CTRL_C_EVENT received\n");
        break;
    case CTRL_CLOSE_EVENT:
        printf("[+] CTRL_CLOSE_EVENT received\n");
        break;
    case CTRL_BREAK_EVENT:
        printf("[+] CTRL_BREAK_EVENT received\n");
        break;
    case CTRL_LOGOFF_EVENT:
        printf("[+] CTRL_LOGOFF_EVENT received\n");
        break;
    case CTRL_SHUTDOWN_EVENT:
        printf("[+] CTRL_SHUTDOWN_EVENT received\n");
        break;
    }

    flag = true;

    return TRUE;
}