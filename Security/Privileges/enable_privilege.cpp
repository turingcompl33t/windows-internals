// enable_privilege.cpp
//
// Enable the specific privilege in current primary token.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include enable_privilege.cpp

#include "privileges.hpp"

#include <iostream>
#include <string_view>

#include <wdl/handle/generic.hpp>

#pragma comment(lib, "advapi32")

using null_handle = wdl::handle::null_handle;

constexpr auto const STATUS_SUCCESS_I = 0x0;
constexpr auto const STATUS_FAILURE_I = 0x1;

constexpr auto const NAME_MAX_LEN = 128;

void error(
	std::string_view msg,
	unsigned long const code = ::GetLastError()
	)
{
	std::cout << "[-] " << msg
		<< " Code: " << code
		<< std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "[-] Invalid arguments\n"
            << "[-] Usage: " << argv[0] << " <PRIVILEGE NAME>\n";
        return STATUS_FAILURE_I;
    }

    wchar_t privilege_name[NAME_MAX_LEN];
    auto converted = size_t{};
    ::mbstowcs_s(&converted, privilege_name, argv[1], NAME_MAX_LEN-1);
    if (converted == 0 || converted == NAME_MAX_LEN)
    {
        std::cout << "[-] Invalid name provided\n";
        return STATUS_FAILURE_I;
    }

    auto this_token = null_handle{};
    if (!::OpenProcessToken(
        ::GetCurrentProcess(), 
        TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, 
        this_token.put()))
    {
        error("OpenProcessToken() failed");
        return STATUS_FAILURE_I;
    }

    auto status = STATUS_SUCCESS_I;
    if (!set_privilege(
        this_token.get(), 
        privilege_name, 
        privilege_action::enable))
    {
        error("Failed to enable privilege");
        status = STATUS_FAILURE_I;
    }
    else
    {
        std::cout << "Success!\n";
    }

    return status;
}