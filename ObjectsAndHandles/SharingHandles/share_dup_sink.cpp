// share_dup_sink.cpp
//
// Demonstrates sharing via kernel object duplication.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 share_dup_sink.cpp

#include <windows.h>
#include <cstdio>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

struct MESSAGE
{
    unsigned long handle_num;
};

int main()
{
    puts("[SINK] Process started; reading from STDIN");

    auto m = MESSAGE{};

    // read from stdin to determine handle number 
    if (!::ReadFile(
        ::GetStdHandle(STD_INPUT_HANDLE), 
        &m,
        sizeof(MESSAGE),
        nullptr,
        nullptr))
    {
        printf("[SINK] Failed to read from pipe; GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    printf("[SINK] Read handle value %u from pipe\n", m.handle_num);

    // handle has been duplicated to our process, translate index to handle
    auto event = ::ULongToHandle(m.handle_num);

    if (!::SetEvent(event))
    {
        printf("[SINK] Failed to signal event; GLE: %u\n", ::GetLastError());
    }
    else
    {
        puts("[SINK] Successfully signaled event; process exiting");
    }

    ::CloseHandle(event);

    return STATUS_SUCCESS_I;
}