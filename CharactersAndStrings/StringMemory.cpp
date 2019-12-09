// StringMemory.cpp
// A brief look at strings under the hood.

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <string.h>

#include <cstdio>

INT wmain(VOID)
{
    const CHAR AnsiString[] = "ABCDE" ;
    const WCHAR WideString[] = L"ABCDE";

    const auto AnsiStringLength = strnlen_s(AnsiString, 5);
    const auto WideStringLength = wcsnlen_s(WideString, 5);

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

    return 0;
}