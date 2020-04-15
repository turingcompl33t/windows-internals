// single_instance.cpp
//
// Using named kernel objects to single-instance an application.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include single_instance.cpp

#include <windows.h>
#include <wdl/handle/generic.hpp>

#include <iostream>
#include <string_view>

using null_handle = wdl::handle::null_handle;

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

// generated with powershell [guid]::NewGuid().ToString()
constexpr const auto MUTEX_NAME = L"e174ed36-6312-47e6-a5cf-dbe287f31711";

void error(
    std::string_view msg,
    unsigned long const code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << std::hex << code << std::dec
        << std::endl;
}

int main()
{
    // create a named mutex object
    auto mutex = null_handle {
        ::CreateMutexW(nullptr, FALSE, MUTEX_NAME) };

    if (!mutex)
    {
        error("CreateMutex() failed");
        return STATUS_FAILURE_I;
    }

    if (::GetLastError() == ERROR_ALREADY_EXISTS)
    {
        std::cout << "[+] Instance already running; press ENTER to exit\n";
        std::cin.get();
        return STATUS_SUCCESS_I;
    }

    // this is the sole running instance
    std::cout << "[+] Sole running instance; press ENTER to exit\n";
    std::cin.get();

    return STATUS_SUCCESS_I;
}