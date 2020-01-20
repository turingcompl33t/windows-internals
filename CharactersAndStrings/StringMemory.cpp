// StringMemory.cpp
// A brief look at strings under the hood.
//
// Build:
//  cl /EHsc /nologo StringMemory.cpp

#include <windows.h>
#include <string.h>
#include <cstdio>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

INT main()
{
    const CHAR AnsiString[]  = "ABCDE" ;
    const WCHAR WideString[] = L"ABCDE";

    const auto AnsiStringLength = strnlen_s(AnsiString, _countof(AnsiString));
    const auto WideStringLength = wcsnlen_s(WideString, _countof(WideString));

    const auto& AnsiBuffer = reinterpret_cast<BYTE*>(const_cast<CHAR*>(AnsiString));
    const auto& WideBuffer = reinterpret_cast<BYTE*>(const_cast<WCHAR*>(WideString));

    puts("Dumping bytes of ANSI string...");
    for (SIZE_T i = 0; i < AnsiStringLength * sizeof(CHAR); ++i)
    {
        printf("0x%02X\n", AnsiBuffer[i]);
    }

    puts("Dumping bytes of wide string...");
    for (SIZE_T i = 0; i < WideStringLength * sizeof(WCHAR); ++i)
    {
        printf("0x%02X\n", WideBuffer[i]);
    }

    return STATUS_SUCCESS_I;
}