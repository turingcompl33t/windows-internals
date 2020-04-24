// use_lock_memory.cpp
//
// Messing with token privileges.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include use_lock_memory.cpp

#include "privileges.hpp"

#include <iostream>
#include <string_view>

#include <wdl/handle/generic.hpp>

#pragma comment(lib, "advapi32")

using null_handle = wdl::handle::null_handle;

constexpr auto const STATUS_SUCCESS_I = 0x0;
constexpr auto const STATUS_FAILURE_I = 0x1;

void error(
	std::string_view msg,
	unsigned long const code = ::GetLastError()
	)
{
	std::cout << "[-] " << msg
		<< " Code: " << code
		<< std::endl;
}

int main()
{
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
    if (!set_privilege(this_token.get(), L"SeLockMemoryPrivilege", true))
    {
        error("Failed to enable SeLockMemory");
        status = STATUS_FAILURE_I;
    }
    else
    {
        std::cout << "Success!\n";
    }

    return status;
}