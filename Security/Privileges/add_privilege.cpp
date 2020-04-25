// add_privilege.cpp
//
// Add the specified privilege to the account of the process' primary token.
//
// Requires elevation.
// Requires logoff in order for change to take effect.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include add_privilege.cpp

#include "privileges.hpp"

#include <iostream>
#include <string_view>

#include <wdl/handle/generic.hpp>
#include <wdl/security/base.hpp>

#pragma comment(lib, "advapi32")

using null_handle = wdl::handle::null_handle;
using lsa_policy_handle = wdl::security::lsa_policy_handle;

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

void nt_error(
    std::string_view    msg,
    unsigned long const code
    )
{
    auto const win32 = ::LsaNtStatusToWinError(code);
    std::cout << "[-] " << msg
        << " Code: " << win32
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

    // open the current process token
    auto this_token = null_handle{};
    if (!::OpenProcessToken(
        ::GetCurrentProcess(), 
        TOKEN_QUERY, 
        this_token.put()))
    {
        error("OpenProcessToken() failed");
        return STATUS_FAILURE_I;
    }

    // grab the user SID from the token
    auto buffer = get_token_information(this_token.get(), TokenUser);
    if (!buffer)
    {
        std::cout << "[-] Failed to query token user information\n";
        return STATUS_FAILURE_I;
    }

    this_token.release();
    auto& token_user = *reinterpret_cast<PTOKEN_USER>(buffer.get());

    // open the LSA policy database 
    auto policy = lsa_policy_handle{};
    auto const status = open_policy(
        nullptr, 
        POLICY_CREATE_ACCOUNT | 
        POLICY_LOOKUP_NAMES, 
        policy.put());
    
    if (!NT_SUCCESS(status))
    {
        nt_error("Failed to acquire LSA policy handle", status);
        return STATUS_FAILURE_I;
    }

    // add the privilege to the specified account
    if (!set_account_privilege(
        policy.get(), 
        token_user.User.Sid, 
        privilege_name, 
        account_privilege_action::add))
    {
        std::cout << "[-] Failed to set account privilege\n";
        return STATUS_FAILURE_I;
    }

    std::cout << "[+] Success!\n";

    return STATUS_SUCCESS_I;
}