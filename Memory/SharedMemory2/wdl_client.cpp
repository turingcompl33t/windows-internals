// wdl_client.cpp
//
// Shared memory with wdl::memory::shared_memory.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include wdl_client.cpp

#include <windows.h>
#include <wdl/memory/shared_memory.hpp>

#include <string>
#include <iostream>
#include <string_view>

using shared_memory = wdl::memory::shared_memory;
using shared_memory_type = wdl::memory::shared_memory_type;

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto MAX_NAME_LEN = 256;

void prompt(long long* count)
{
    std::cout << "[+] Current count = " << *count << '\n'
        << "[+] ENTER to increment (q to quit)\n"
        << ">>";
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "[-] Invalid arguments\n"
            << "[-] Usage: " << argv[0] << " <MAPPING NAME>\n";
        return STATUS_FAILURE_I;
    }

    wchar_t name[MAX_NAME_LEN];
    mbstowcs_s(nullptr, name, argv[1], MAX_NAME_LEN-1);

    auto size = ULARGE_INTEGER{256, 0};
    auto memory = shared_memory{name, &size};
    if (!memory)
    {
        std::cout << "[-] Failed to initialize shared memory\n";
    }
    else
    {
        std::wcout << L"[+] Successfully initialized shared memory object"
            << L" with name: " << std::wstring_view{name, wcslen(name)} << L'\n';
    }

    auto* count = reinterpret_cast<long long*>(memory.begin());
    InterlockedExchange64(count, 0);

    auto input = std::string{};
    
    prompt(count);
    for (;;)
    {
        std::getline(std::cin, input);
        if (input == "q" || input == "quit" || input == "exit")
            break;

        InterlockedIncrement64(count);
        prompt(count);
    }

    std::cout << "[+] Exiting\n";

    return STATUS_SUCCESS_I;
}

