// add_lock_memory.cpp
//
// Messing with token privileges.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include add_lock_memory.cpp

#include "privileges.hpp"

#include <iostream>
#include <string_view>

#include <wdl/handle/generic.hpp>

using null_handle = wdl::handle::null_handle;

#pragma comment(lib, "advapi32")

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
    // open the current process token
    auto this_token = null_handle{};
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, this_token.put()))
    {
        error("OpenProcessToken() failed");
        return STATUS_FAILURE_I;
    }

    auto buffer = get_token_information(this_token.get(), TokenUser);
    if (!buffer)
    {
        std::cout << "[-] Failed to query token user information\n";
        return STATUS_FAILURE_I;
    }

    this_token.release();
    auto& token_user = *reinterpret_cast<PTOKEN_USER>(buffer.get());

    auto policy = LSA_HANDLE{};
    if (!open_policy(nullptr, POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES, &policy))
    {
        std::cout << "[-] Failed to acquire policy handle\n";
        return STATUS_FAILURE_I;
    }

    if (!set_account_privilege(
        policy, 
        token_user.User.Sid, 
        L"SeLockMemoryPrivilege", 
        true))
    {
        std::cout << "[-] Failed to set account privilege\n";
        return STATUS_FAILURE_I;
    }
}