// wdl_client.cpp
//
// Interprocess barrier with wdl::synchronization::interprocess_barrier.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include wdl_client.cpp

#include <windows.h>
#include <wdl/synchronization/interprocess_barrier.hpp>

#include <string>
#include <iostream>
#include <string_view>

using interprocess_barrier 
    = wdl::synchronization::interprocess_barrier;

constexpr auto const STATUS_SUCCESS_I = 0x0;
constexpr auto const STATUS_FAILURE_I = 0x1;

constexpr const auto MAX_NAME_LEN = 256;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "[-] Invalid arguments\n"
            << "[-] Usage: " << argv[0]
            << " <BARRIER NAME> <COUNT>\n";
        return STATUS_FAILURE_I;
    }

    wchar_t name[MAX_NAME_LEN];
    mbstowcs_s(nullptr, name, argv[1], MAX_NAME_LEN-1);

    auto count = long{};
    try
    {
        count = std::stol(argv[2]);
    }
    catch (std::exception const&)
    {
        std::cout << "[-] Invalid arguments; failed to parse\n";
        return STATUS_FAILURE_I;
    }

    std::wcout << L"[+] Creating interprocess barrier\n"
        << L"\tName:  " << std::wstring_view{name, wcslen(name)} << L'\n'
        << L"\tCount: " << count << L'\n';

    auto barrier = interprocess_barrier{name, count};

    std::cout << "[+] Successfully initialized barrier; entering...\n";

    barrier.enter();

    std::cout << "[+] Released from barrier; exiting\n";

    return STATUS_SUCCESS_I;
}
