// share_dup_sink.cpp
//
// Demonstrates sharing via kernel object duplication.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 share_dup_sink.cpp

#include <windows.h>
#include <cstdio>

struct MESSAGE
{
    ULONG HandleValue;
};

int main(int argc, char* argv[])
{
    puts("[SINK] Process started; reading from STDIN");

    MESSAGE m;

    if (!::ReadFile(
        ::GetStdHandle(STD_INPUT_HANDLE), 
        &m,
        sizeof(MESSAGE),
        nullptr,
        nullptr
        )
    )
    {
        printf("[SINK] Failed to read from pipe; GLE: %u\n", ::GetLastError());
        return 1;
    }

    printf("[SINK] Read handle value %u from pipe\n", m.HandleValue);

    HANDLE hEvent = ::ULongToHandle(m.HandleValue);
    if (!::SetEvent(hEvent))
    {
        printf("[SINK] Failed to signal event; GLE: %u\n", ::GetLastError());
    }

    puts("[SINK] Successfully signaled event; process exiting");

    return 0;
}