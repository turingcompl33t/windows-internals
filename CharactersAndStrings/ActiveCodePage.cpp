// ActiveCodePage.cpp
// Querying the currently active Windows code page.
//
// Build:
//  cl /EHsc /nologo ActiveCodePage.cpp

#include <windows.h>
#include <tchar.h>
#include <cstdio>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

INT _tmain()
{
    auto acp = ::GetACP();
    _tprintf(_T("[+] Current active code page: %u\n"), acp);

    return STATUS_SUCCESS_I;
}