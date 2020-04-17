// loader.cpp
//
// Trying to load two fixed-base DLLs at the same address.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include loader.cpp

#include <windows.h>
#include <wdl/handle/module.hpp>

#include <iostream>
#include <string>
#include <string_view>

using module_handle = wdl::handle::module_handle; 

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

void error(
    std::string_view msg,
    unsigned long const code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << " Code: " << std::hex << code << std::dec
        << std::endl;
}

module_handle load(std::wstring const& module_name)
{
    auto lib = module_handle{ ::LoadLibraryW(module_name.c_str()) };
    if (!lib)
    {
        std::wcout << L"[-] Failed to load module "
            << module_name << L" Code: " << ::GetLastError()
            << L"\n";
    }
    else
    {
        std::wcout << L"[+] Successfully loaded module "
            << module_name
            << L"\n";
    }

    return lib;
}

int main()
{
    auto lib1 = load(L"lib1");
    auto lib2 = load(L"lib2");

    return STATUS_SUCCESS_I; 
}