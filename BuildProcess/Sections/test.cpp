// test.cpp
//
// Looking at PE image sections.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 test.cpp

#include <iostream>

int       g_readwrite = 1337;
int const g_readonly  = 1337;

char const* g_str = "global string";

int main()
{
    char const* l_str = "local string";

    std::cout << "g_readwrite = " << g_readwrite << '\n';
    std::cout << "g_readonly  = " << g_readonly << '\n';

    std::cout << "g_str = " << g_str << '\n';
    std::cout << "l_str = " << l_str << '\n';

    return 0;
}