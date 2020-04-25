// string_security_descriptor.cpp
//
// Quick and dirty method to view security descriptor.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 string_security_descriptor.cpp

#include <windows.h>
#include <sddl.h>
#include <cstdio>

#pragma comment(lib, "advapi32")

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

int main()
{
    auto token = HANDLE{};
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        ::printf("[-] Failed to open process token\n");
        ::printf("[-] GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    auto required_size = 0ul;

    ::GetTokenInformation(token, TokenDefaultDacl, NULL, 0, &required_size);

    auto* default_dacl = reinterpret_cast<TOKEN_DEFAULT_DACL*>(
        ::LocalAlloc(LPTR, required_size));

    if (!default_dacl)
    {
        printf("[-] Failed to allocate memory\n");
        printf("[-] GLE: %u\n", ::GetLastError());
        ::CloseHandle(token);
        return STATUS_FAILURE_I;
    }

    auto sd = SECURITY_DESCRIPTOR{};
    wchar_t* string_sid;

    if (::GetTokenInformation(token, TokenDefaultDacl, default_dacl, required_size, &required_size) &&
        ::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) &&
        ::SetSecurityDescriptorDacl(&sd, TRUE, default_dacl->DefaultDacl, FALSE) &&
        ::ConvertSecurityDescriptorToStringSecurityDescriptorW(
            &sd,
            SDDL_REVISION_1, 
            DACL_SECURITY_INFORMATION, 
            &string_sid, 
            NULL)
    ) 
    {
        ::wprintf(string_sid);
        ::LocalFree(string_sid);
    }

    ::LocalFree(default_dacl);
    ::CloseHandle(token);

    return STATUS_SUCCESS_I;
}