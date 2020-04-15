// share_named_child.cpp
//
// Demonstrates kernel object handle sharing via named object.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 share_named_child.cpp

#include <windows.h>
#include <string>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto EVENT_NAME = L"SharedNameEvent";

int main()
{
    puts("[CHILD] Process started");

    auto event = ::OpenEventW(
        EVENT_ALL_ACCESS,
        FALSE,
        EVENT_NAME);

    if (NULL == event)
    {
        printf("[CHILD] Failed to acquire handle to event; GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    puts("[CHILD] Successfully acquired handle to event");

    ::WaitForSingleObject(event, INFINITE);

    puts("[CHILD] Event signaled; process exiting");

    ::CloseHandle(event);

    return STATUS_SUCCESS_I;
}