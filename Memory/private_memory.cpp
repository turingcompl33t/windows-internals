// private_memory.cpp
//
// Simple shared memory example via executable memory protection attributes;
// baseline for example, without shared data section defined.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 private_memory.cpp

#include <windows.h>

#include <string>
#include <iostream>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

volatile unsigned long g_shared_data = 0;

void prompt()
{
    std::cout << "[+] Current Value = " << g_shared_data << '\n';
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

        InterlockedIncrement(&g_shared_data);
        prompt();
    }

    std::cout << "[+] Exiting\n";
    return STATUS_SUCCESS_I;
}