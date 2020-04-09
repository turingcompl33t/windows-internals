// active_code_page.cpp
//
// Querying the currently active Windows code page.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 active_code_page.cpp

#include <windows.h>
#include <cstdio>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

int main()
{
    printf("[+] Current active code page: %u\n", GetACP());
    return STATUS_SUCCESS_I;
}