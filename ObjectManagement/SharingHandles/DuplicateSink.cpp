// DuplicateSink.cpp
// Sink application for handle duplication.

#include <windows.h>

#include <cstdio>

typedef struct _MESSAGE
{
    ULONG HandleValue;
} MESSAGE;

INT main(INT argc, PCHAR argv[])
{
    puts("[SINK] Sink process started; reading from STDIN");

    MESSAGE m;

    if (!::ReadFile(
        GetStdHandle(STD_INPUT_HANDLE), 
        &m,
        sizeof(MESSAGE),
        nullptr,
        nullptr
        )
    )
    {
        printf("[SINK] Failed to read from pipe; GLE: %u\n", GetLastError());
        return 1;
    }

    printf("[SINK] Read handle value %u from pipe\n", m.HandleValue);

    HANDLE hEvent = ULongToHandle(m.HandleValue);
    if (!::SetEvent(hEvent))
    {
        printf("[SINK] Failed to signal event; GLE: %u\n", GetLastError());
    }

    puts("[SINK] Sink process exiting");

    return 0;
}