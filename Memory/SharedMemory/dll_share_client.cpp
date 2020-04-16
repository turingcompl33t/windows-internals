// dll_share_client.cpp
//
// Demo of simple data sharing via DLL.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 dll_share_client.cpp

#include <windows.h>

#include <string>
#include <iostream>

#pragma comment(lib, "data_share_server")

__declspec(dllimport) unsigned long g_counter;

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

void prompt()
{
    std::cout << "[+] Current Value = " << g_counter << '\n';
    std::cout << "[+] ENTER to Increment (q to quit)\n";
    std::cout << ">> ";
}

int main()
{
    auto input = std::string{};

    prompt();
    while (std::getline(std::cin, input))
    {
        if (input == "q" || input == "quit" || input == "exit")
        {
            break;
        }

        InterlockedIncrement(&g_counter);
        prompt();
    }

    std::cout << "[+] Exiting\n";
    return STATUS_SUCCESS_I;
}