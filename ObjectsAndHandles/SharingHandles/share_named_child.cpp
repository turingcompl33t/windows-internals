// share_named_child.cpp
//
// Demonstrates kernel object handle sharing by name.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 share_named_child.cpp

#include <windows.h>

#include <string>

constexpr auto EventName = L"SharedNameEvent";

int main()
{
    puts("[CHILD] Process started");

    HANDLE hEvent;

    hEvent = ::OpenEventW(
        EVENT_ALL_ACCESS,
        FALSE,
        EventName
        );

    if (NULL == hEvent)
    {
        printf("[CHILD] Failed to acquire handle to event; GLE: %u\n", ::GetLastError());
        return 1;
    }

    puts("[CHILD] Successfully acquired handle to event");

    ::WaitForSingleObject(hEvent, INFINITE);

    puts("[CHILD] Event signaled; process exiting");

    ::CloseHandle(hEvent);

    return 0;
}