// string_memory.cpp
//
// A brief look at strings under the hood.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 string_memory.cpp

#include <windows.h>
#include <string.h>
#include <cstdio>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

int main()
{
    const char ansi[]       = "ABCDE" ;
    const wchar_t unicode[] = L"ABCDE";

    const auto ansi_len = strnlen_s(ansi, _countof(ansi));
    const auto unicode_len = wcsnlen_s(unicode, _countof(unicode));

    const auto& ansi_buffer = reinterpret_cast<BYTE*>(const_cast<char*>(ansi));
    const auto& unicode_buffer = reinterpret_cast<BYTE*>(const_cast<wchar_t*>(unicode));

    puts("Dumping bytes of ANSI string...");
    for (auto i = 0u; i < ansi_len * sizeof(CHAR); ++i)
    {
        printf("0x%02X\n", ansi_buffer[i]);
    }

    puts("Dumping bytes of wide string...");
    for (auto i = 0u; i < unicode_len * sizeof(WCHAR); ++i)
    {
        printf("0x%02X\n", unicode_buffer[i]);
    }

    return STATUS_SUCCESS_I;
}