// priviliges.hpp
//
// Messing with token privileges.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include privileges.cpp

#pragma once

#include <windows.h>
#include <ntsecapi.h>
#include <sddl.h>

#include <memory>

constexpr bool NT_SUCCESS(NTSTATUS const status)
{
    return status >= 0;
}

void init_lsa_string(PLSA_UNICODE_STRING lsa_string, LPWSTR string)
{
    if (nullptr == string) 
    {
        lsa_string->Buffer = NULL;
        lsa_string->Length = 0;
        lsa_string->MaximumLength = 0;
        return;
    }

    auto const length = ::wcslen(string);

    lsa_string->Buffer = string;
    lsa_string->Length 
        = static_cast<unsigned short>(length * sizeof(wchar_t));
    lsa_string->MaximumLength 
        = static_cast<unsigned short>((length + 1) * sizeof(wchar_t));
}

NTSTATUS open_policy(
    wchar_t*            server_name, 
    unsigned long const desired_access, 
    PLSA_HANDLE         policy_handle
    )
{
    auto object_attrs = LSA_OBJECT_ATTRIBUTES{};
    auto server_string = LSA_UNICODE_STRING{};
    auto server = PLSA_UNICODE_STRING{};

    ZeroMemory(&object_attrs, sizeof(object_attrs));

    if (server_name != NULL) 
    {
        init_lsa_string(&server_string, server_name);
        server = &server_string;
    }

    return NT_SUCCESS(
        ::LsaOpenPolicy(
            server,
            &object_attrs,
            desired_access,
            policy_handle));
}

NTSTATUS set_account_privilege(
    LSA_HANDLE policy, 
    PSID       account_sid, 
    wchar_t*   privilege_name, 
    bool       enable
    )
{
    auto privilege_string = LSA_UNICODE_STRING{};
    init_lsa_string(&privilege_string, privilege_name);

    auto status = NTSTATUS{};
    if (enable) 
    {
        status = ::LsaAddAccountRights(
            policy, 
            account_sid,      
            &privilege_string,  
            1);
    }
    else 
    {
        status = ::LsaRemoveAccountRights(
            policy, 
            account_sid, 
            FALSE,  
            &privilege_string,  
            1);
    }

    return NT_SUCCESS(status);
}

bool set_privilege(
    HANDLE         token, 
    wchar_t const* name, 
    bool           enable
    )
{
	auto luid       = LUID{};
	auto privileges = TOKEN_PRIVILEGES{};

	if (!::LookupPrivilegeValueW(nullptr, name, &luid))
	{
		return false;
	}

	privileges.PrivilegeCount = 1;
	privileges.Privileges[0].Luid = luid;

    if (enable)
    {
	    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    }
    else
    {
	    privileges.Privileges[0].Attributes = 0;
    }

	if (!::AdjustTokenPrivileges(
		token, 
		FALSE, 
		&privileges, 
		sizeof(TOKEN_PRIVILEGES), 
		nullptr, nullptr))
	{
		return false;
	}

	return ::GetLastError() != ERROR_NOT_ALL_ASSIGNED;
}

std::unique_ptr<char[]> get_token_information(
    HANDLE                  token,
    TOKEN_INFORMATION_CLASS info_class
    )
{
    auto required_size = unsigned long{};
    if (!::GetTokenInformation(token, info_class, nullptr, 0, &required_size)
        && ::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        return std::unique_ptr<char[]>{};
    }

    auto buffer = std::make_unique<char[]>(required_size);
    if (!::GetTokenInformation(
        token, 
        info_class,
        buffer.get(),
        required_size,
        &required_size))
    {
        return std::unique_ptr<char[]>{};
    }

    return buffer;
}