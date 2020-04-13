// shared_memory.cpp
//
// Simple shared memory example via executable memory protection attributes.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 shared_memory.cpp

#include <windows.h>

#include <string>
#include <iostream>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

// tell the linker to insert a new section into the executable
// within this new section, define some global shared data
#pragma data_seg("shared")
volatile unsigned long g_shared_data = 0;
#pragma data_seg()

// tell the linker that the section "shared" should
// have the read and write memory protection properties,
// and that the section should be shared across instances
#pragma comment(linker, "/section:shared,RWS")

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