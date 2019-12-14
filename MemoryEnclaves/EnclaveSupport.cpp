// EnclaveSupport.cpp
// Simple program to determine enclave support for current system.

#include <windows.h>

#include <cstdio>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

INT main(INT argc, PUCHAR argv[])
{
    if (argc != 1)
    {
        printf("[-] Invalid arguments\n");
        printf("[-] Usage: %s\n", argv[0]);
        return STATUS_FAILURE_I;
    }

    auto vbsEnclaveSupported = IsEnclaveTypeSupported(ENCLAVE_TYPE_VBS);
    auto sgxEnclaveSupported = IsEnclaveTypeSupported(ENCLAVE_TYPE_SGX);

    printf("[+] VBS Enclave Support: %s\n", vbsEnclaveSupported ? "ENABLED" : "DISABLED");
    printf("[+] SGX Enclave Support: %s\n", sgxEnclaveSupported ? "ENABLED" : "DISABLED");

    return STATUS_SUCCESS_I;
}