// DebugMe.cpp
// A Demo application to be compiled as 32-bit executable and debugged.
//
// 32-bit Build:
//  (from x86 native tools command prompt)
//  cl /EHsc /nologo /Fe:DebugMe86 DebugMe.cpp /link /machine:x86
//
// 64-bit Build:
//  (from x64 native tools command prompt)
//  cl /EHsc /nologo /Fe:DebugMe64 DebugMe.cpp /link /machine:x64

#include <windows.h>
#include <iostream>

int main()
{
    __debugbreak();

    HANDLE hEvent;

    hEvent = ::CreateEventW(
        nullptr,
        FALSE,
        FALSE,
        nullptr
    );

    __debugbreak();

    if (NULL == hEvent)
    {
        std::cout << "CreateEvent() failed\n";
        std::cout << "GLE: " << ::GetLastError() << '\n';
        return 1;
    }

    ::CloseHandle(hEvent);

    return 0;
}