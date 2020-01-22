// StringSecurityDescriptor.cpp
// Quick and dirty method to view security descriptor.

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <sddl.h>
#include <cstdio>
#include <tchar.h>

#pragma comment(lib, "advapi32")

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

int wmain()
{
    HANDLE token;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        printf("[-] Failed to open process token\n");
        printf("[-] GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    DWORD requiredSize = 0;

    ::GetTokenInformation(token, TokenDefaultDacl, NULL, 0, &requiredSize);

    auto* defaultDacl = reinterpret_cast<TOKEN_DEFAULT_DACL*>(
        ::LocalAlloc(LPTR, requiredSize)
    );

    if (!defaultDacl)
    {
        printf("[-] Failed to allocate memory\n");
        printf("[-] GLE: %u\n", ::GetLastError());
        ::CloseHandle(token);
        return STATUS_FAILURE_I;
    }

    SECURITY_DESCRIPTOR sd;
    LPTSTR stringSid;

    if (::GetTokenInformation(token, TokenDefaultDacl, defaultDacl, requiredSize, &requiredSize) &&
        ::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) &&
        ::SetSecurityDescriptorDacl(&sd, TRUE, defaultDacl->DefaultDacl, FALSE) &&
        ::ConvertSecurityDescriptorToStringSecurityDescriptor(
            &sd,
            SDDL_REVISION_1, 
            DACL_SECURITY_INFORMATION, 
            &stringSid, 
            NULL)
    ) 
    {
        _tprintf(stringSid);

        ::LocalFree(stringSid);
    }

    ::LocalFree(defaultDacl);
    ::CloseHandle(token);

    return STATUS_SUCCESS_I;
}