// AbnormalTermination.cpp
// Demonstration of abnormal termination conditions.

#include <windows.h>
#include <cstdio>
#include <string>
#include <stdexcept>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

VOID TryAbnormalTermination(ULONG code);

INT main(INT argc, CHAR* argv[])
{
    if (argc != 2)
    {
        printf("[-] Invalid arguments\n");
        printf("[-] Usage: %s <CODE>\n", argv[0]);
        return STATUS_FAILURE_I;
    }

    ULONG code;

    // using C++ standard exceptions in a program
    // meant to demonstrate an aspect of SEH... 
    try
    {
        code = std::stoul(argv[1]);
    }
    catch (const std::exception& e)
    {
        printf("[-] Failed to parse code argument\n");
        printf("[-] %s\n", e.what());
        return STATUS_FAILURE_I;
    }

    // need to move the abnormal termination test to a separate 
    // function because only one form of exception handling
    // (SEH or C++) is permitted per function
    TryAbnormalTermination(code);

    return STATUS_FAILURE_I;
}

VOID TryAbnormalTermination(ULONG code)
{
    BOOL status;

    __try
    {
        if (0 == code)
        {
            printf("[+] EXITING __try BLOCK WITH return\n");
            return;
        }
        else if (1 == code)
        {
            printf("[+] EXITING __try BLOCK WITH __leave\n");
            __leave;
        }
        else 
        {
            // fall through
            printf("[+] EXITING __try BLOCK WITH FALL-THROUGH\n");
        }
    }
    __finally
    {
        // termination handler

        // under what conditions was the __try block exited?
        status = AbnormalTermination();
        printf("[+] TERMINATION HANDLER, ABNORMAL: %s\n", status ? "TRUE" : "FALSE");
    }
}
