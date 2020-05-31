// CppWalk.cpp
// Recursive directory walk utilizing C++17 filesystem features.
// 
// Compile:
//  cl /EHsc /nologo /std:c++17 CppWalk.cpp

#include <windows.h>

#include <cstdio>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

INT main(INT argc, PCHAR argv[])
{
    if (argc != 2)
    {
        std::cout << "[-] Invalid arguments\n";
        std::cout << "[-] Usage: " << argv[0] << " <DIRWALK ROOT>\n";
        return STATUS_FAILURE_I;
    }

    std::cout << "[+] Starting recursive directory walk at root: ";
    std::cout << argv[1] << '\n';

    for (auto i = fs::recursive_directory_iterator(argv[1]);
             i != fs::recursive_directory_iterator();
           ++i
    ) 
    {
        std::cout << std::string(i.depth(), ' ') << *i;
        std::cout << '\n';
    }
    std::cout << std::flush;
}