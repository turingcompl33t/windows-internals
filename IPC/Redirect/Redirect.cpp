// Redirect.cpp
// Redirect IO between two processes using anonymous pipes.
//
// Compile:
//  cl /EHsc /nologo Redirect.cpp

#define UNICODE
#define _UNICODE

#include <windows.h>

#include <cstdio>
#include <string>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr auto N_PROCESSES         = 2;
constexpr auto CMDLINE_BUFFER_SIZE = 256;
constexpr auto TEMP_BUFFER_SIZE    = 16; 

VOID CreateCommandLine(
    WCHAR program[], 
    HANDLE hRead, 
    HANDLE hWrite,
    WCHAR outbuffer[],
    rsize_t outbufferLength
    );

INT wmain(INT argc, PWCHAR argv[])
{
    if (argc != 3)
    {
        printf("[-] Invalid arguments\n");
        printf("[-] Usage: %ws <PROGRAM1> <PROGRAM2>\n", argv[0]);
        return STATUS_FAILURE_I;
    }

    auto rollback = 0;
    auto status = STATUS_SUCCESS_I;

    // pipe security attributes for both pipes
    // specify that the handles are inheritable
    SECURITY_ATTRIBUTES PipeSA = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

    HANDLE hRead0;
    HANDLE hRead1;
    HANDLE hWrite0;
    HANDLE hWrite1;
    PROCESS_INFORMATION ProcInfo0;
    PROCESS_INFORMATION ProcInfo1;

    HANDLE ProcessHandles[N_PROCESSES];

    WCHAR Program0CmdLine[CMDLINE_BUFFER_SIZE];
    WCHAR Program1CmdLine[CMDLINE_BUFFER_SIZE];

    // full duplex comms
    // Pipe0: Program0 -> Program1
    // Pipe1: Program0 <- Program1

    if (!CreatePipe(&hRead0, &hWrite0, &PipeSA, 0) ||
        !CreatePipe(&hRead1, &hWrite1, &PipeSA, 0))
    {
        printf("[-] Failed to create anonymous pipes (CreatePipe())\n");
        printf("GLE: %u\n", GetLastError());
        status = STATUS_FAILURE_I;
        goto CLEANUP;
    }

    // pipes created successfully
    rollback = 1;

    // construct command lines
    CreateCommandLine(argv[1], hRead1, hWrite0, Program0CmdLine, CMDLINE_BUFFER_SIZE);
    CreateCommandLine(argv[2], hRead0, hWrite1, Program1CmdLine, CMDLINE_BUFFER_SIZE);

    printf("Starting program 0 with command line: %ws\n", Program0CmdLine);
    printf("Starting program 1 with command line: %ws\n", Program1CmdLine);

    if (!CreateProcess(
        nullptr,
        Program0CmdLine,
        nullptr,
        nullptr, 
        TRUE,                  // inherit handles
        CREATE_NEW_CONSOLE,    // create a new console for this program
        nullptr,
        nullptr,
        nullptr,
        &ProcInfo0)
    )
    {
        printf("Failed to start Program 0 (CreateProcess())\n");
        printf("GLE: %u\n", GetLastError());
        status = STATUS_FAILURE_I;
        goto CLEANUP;
    }

    CloseHandle(ProcInfo0.hThread);
    ProcessHandles[0] = ProcInfo0.hProcess;

    // CreateProcess() #1 succeeded
    rollback = 2;

    if (!CreateProcess(
        nullptr,
        Program1CmdLine,
        nullptr,
        nullptr,
        TRUE,                  // inherit handles
        CREATE_NEW_CONSOLE,    // create a new console for this program
        nullptr,
        nullptr,
        nullptr,
        &ProcInfo1)
    )
    {
        printf("Failed to start Program 1 (CreateProcess())\n");
        printf("GLE: %u\n", GetLastError());
        status = STATUS_FAILURE_I;
        goto CLEANUP;
    }

    CloseHandle(ProcInfo1.hThread);
    ProcessHandles[1] = ProcInfo1.hProcess;

    // CreateProcess() #2 succeeded
    rollback = 3;

    // wait for processes to exit
    WaitForMultipleObjects(N_PROCESSES, ProcessHandles, TRUE, INFINITE);

CLEANUP:
    switch(rollback)
    {
        case 3:
            CloseHandle(ProcInfo1.hProcess);
        case 2:
            CloseHandle(ProcInfo0.hProcess);
        case 1:
            CloseHandle(hRead0);
            CloseHandle(hRead1);
            CloseHandle(hWrite0);
            CloseHandle(hWrite1);
        case 0:
            break;
    }

    return status;
}

// create command line for application startup
VOID CreateCommandLine(
    WCHAR program[], 
    HANDLE hRead, 
    HANDLE hWrite,
    WCHAR outbuffer[],
    rsize_t outbufferLength
    )
{
    RtlZeroMemory(outbuffer, outbufferLength);

    DWORD dwRead = HandleToULong(hRead);
    DWORD dwWrite = HandleToULong(hWrite);

    // TODO: add more robust error handling here...
    WCHAR ArgsBuffer[TEMP_BUFFER_SIZE];
    swprintf_s(ArgsBuffer, TEMP_BUFFER_SIZE, L" %u %u", dwRead, dwWrite);

    // TODO: and here...
    wcscpy_s(outbuffer, outbufferLength, program);
    wcsncat_s(outbuffer, outbufferLength, ArgsBuffer, TEMP_BUFFER_SIZE);
}