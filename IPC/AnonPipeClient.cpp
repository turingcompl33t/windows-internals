// AnonPipeClient.cpp
// A simple application to demonstrate IPC via anonymous pipes.

#define UNICODE
#define _UNICODE

#include <windows.h>

#include <iostream>
#include <string_view>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

VOID LogInfo(const std::string_view msg);
VOID LogWarning(const std::string_view msg);
VOID LogError(const std::string_view msg, DWORD err = ::GetLastError());

INT main(INT argc, PCHAR argv[])
{
    if (argc != 1)
    {
        LogWarning("Invalid arguments");
        return STATUS_FAILURE_I;
    }

    LogInfo("Pipe client launched; waiting for input");

    // no need to close this because it is std handle
    HANDLE hPipeIn = GetStdHandle(STD_INPUT_HANDLE);
    CHAR buffer[512];

    for (;;)
    {
        if(!ReadFile(hPipeIn, buffer, 512, nullptr, nullptr))
        {
            std::cout << "GLE: " << GetLastError() << std::endl;
        }
        std::cout << "GOT: " << buffer << std::endl;
    }

    return STATUS_SUCCESS_I;
}

VOID LogInfo(const std::string_view msg)
{
    std::cout << "[+] " << msg << std::endl;
}

VOID LogWarning(const std::string_view msg)
{
    std::cout << "[-] " << msg << std::endl;
}

VOID LogError(const std::string_view msg, DWORD err)
{
    std::cout << "[!] " << msg << '\n';
    std::cout << "[!] " << "GLE: " << err << '\n';
    std::cout << std::endl;
}