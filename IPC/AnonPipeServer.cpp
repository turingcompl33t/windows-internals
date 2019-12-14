// AnonPipeServer.cpp
// A simple application to demonstrate IPC via anonymous pipes.

#define UNICODE
#define _UNICODE

#include <windows.h>

#include <string>
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
    
    auto rollback = 0;
    auto status = STATUS_SUCCESS_I;

    WCHAR ClientApp[] = L"AnonPipeClient.exe";

    // CRITICAL: necessary to specify that handles returned
    // by call to CreatePipe are inheritable
    SECURITY_ATTRIBUTES PipeSA = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

    HANDLE hPipeRead;
    HANDLE hPipeWrite;
    PROCESS_INFORMATION ProcInfo;
    STARTUPINFO StartupInfo;

    std::string input {};

    GetStartupInfo(&StartupInfo);

    if (!CreatePipe(&hPipeRead, &hPipeWrite, &PipeSA, 0))
    {
        LogError("Failed to create anonymous pipe (CreatePipe())");
        status = STATUS_FAILURE_I;
        goto CLEANUP;
    }

    // pipe created
    rollback = 1;

    StartupInfo.hStdInput = hPipeRead;
    StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    StartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    StartupInfo.dwFlags = STARTF_USESTDHANDLES;

    if (!CreateProcess(
        nullptr, 
        ClientApp,
        nullptr, 
        nullptr, 
        TRUE,                     // inherit handles
        0, 
        nullptr, 
        nullptr, 
        &StartupInfo,             // specify handles 
        &ProcInfo)
    )
    {
        LogError("Failed to create new process (CreateProcess())");
        status = STATUS_FAILURE_I;
        goto CLEANUP;
    }

    CloseHandle(hPipeRead);

    // process creation succeeded
    rollback = 2;

    for (;;)
    {
        std::getline(std::cin, input);
        WriteFile(hPipeWrite, input.c_str(), input.length(), nullptr, nullptr);
    }

CLEANUP:

    switch (rollback)
    {   
        case 2:
            CloseHandle(ProcInfo.hThread);
            CloseHandle(ProcInfo.hProcess);
        case 1:
            CloseHandle(hPipeWrite);
        case 0:
            break;
    }

    return status;
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