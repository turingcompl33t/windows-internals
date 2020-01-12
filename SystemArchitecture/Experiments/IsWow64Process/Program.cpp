// Program.cpp
// Simple application to query the WoW64 status of itself.

#include <windows.h>
#include <iostream>
#include <cstdio>

int main()
{
    BOOL result;
    if (!::IsWow64Process(::GetCurrentProcess(), &result))
    {
        std::cout << "IsWow64Process() failed\n";
        std::cout << "GLE: " << GetLastError() << '\n';
        return 1;
    }

    printf("IsWoW64Process: %s\n", result ? "TRUE" : "FALSE");

    return 0;
}