// EchoClient.cpp
// Dead simple app - just echo STDIN to STDOUT.
//
// Compile:
//  cl /EHsc /nologo /MT EchoClient.cpp

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <process.h>

#include <cstdio>
#include <string>
#include <stdexcept>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr auto BUFFER_SIZE = 256;

unsigned WINAPI ThreadFn(PVOID pArg);

// arguments passed to the thread
// that handles pipe IO
typedef struct _TARGS
{
    HANDLE hRead;
    HANDLE hWrite;
} TARGS, *PTARGS; 

INT wmain(INT argc, PWCHAR argv[])
{
    if (argc != 3)
    {
        printf("[-] Invalid arguments\n");
        printf("[-] Usage: %ws <READPIPE HANDLE> <WRITEPIPE HANDLE>\n", argv[0]);
        return STATUS_FAILURE_I;
    }

    for (;;)
    {
        printf("herer\n");
    }

    auto status = STATUS_SUCCESS_I;

    HANDLE hReadFromPipeThread;
    HANDLE hWriteToPipeThread;
    HANDLE ThreadHandles[2];

    HANDLE hReadPipe; 
    HANDLE hWritePipe;
    
    try
    {
        hReadPipe = ULongToHandle(std::stoul(argv[1]));
        hWritePipe = ULongToHandle(std::stoul(argv[2]));
    }
    catch (std::out_of_range)
    {
        printf("[-] Failed to parse command line (out of range)\n");
        status = STATUS_FAILURE_I;
        goto CLEANUP;
    }
    catch (std::invalid_argument)
    {
        printf("[-] Failed to parse command line (invalid argument)\n");
        status = STATUS_FAILURE_I;
        goto CLEANUP;
    }

    // Thread0 reads from pipe, writes to STDOUT
    TARGS ReadFromPipeThreadArgs;
    ReadFromPipeThreadArgs.hRead  = hReadPipe;
    ReadFromPipeThreadArgs.hWrite = GetStdHandle(STD_OUTPUT_HANDLE);

    // Thread1 reads from STDIN, writes to pipe
    TARGS WriteToPipeThreadArgs;
    WriteToPipeThreadArgs.hRead = GetStdHandle(STD_INPUT_HANDLE);
    WriteToPipeThreadArgs.hWrite = hWritePipe;

    hReadFromPipeThread = (HANDLE) _beginthreadex(nullptr, 0, ThreadFn, &ReadFromPipeThreadArgs, 0, nullptr);
    hWriteToPipeThread = (HANDLE) _beginthreadex(nullptr, 0, ThreadFn, &WriteToPipeThreadArgs, 0, nullptr);

    ThreadHandles[0] = hReadFromPipeThread;
    ThreadHandles[1] = hWriteToPipeThread;

    // wait for threads to exit
    WaitForMultipleObjects(2, ThreadHandles, TRUE, INFINITE);

CLEANUP:

    return status;
}

// handle IO
unsigned WINAPI ThreadFn(PVOID pArg)
{   
    // unpack arguments
    auto a = static_cast<PTARGS>(pArg);
    auto hRead = a->hRead;
    auto hWrite = a->hWrite;

    DWORD dwBytesRead;
    WCHAR buffer[BUFFER_SIZE];
    for (;;)
    {
        // block on read
        ReadFile(hRead, buffer, BUFFER_SIZE, &dwBytesRead, nullptr);

        // and write
        WriteFile(hWrite, buffer, dwBytesRead, nullptr, nullptr);
    }

    return STATUS_SUCCESS_I;
}
