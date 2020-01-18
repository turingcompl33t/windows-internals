// ThreadName.cpp
// Demo of new Windows 10 thread "naming" feature.

#define UNICODE
#define _UNICODE

#include <windows.h>

#include <cstdio>
#include <string>
#include <stdexcept>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

INT wmain(INT argc, PWCHAR argv[])
{
    if (argc < 2 || argc > 3)
    {
        printf("[-] Invalid arguments\n");
        printf("[-] Usage: %ws <THREAD ID> [DESCRIPTION]\n", argv[0]);
        return STATUS_FAILURE_I;
    }

    DWORD ThreadId;
    const std::wstring ThreadIdString { argv[1] };

    try
    {
        ThreadId = std::stoul(ThreadIdString);
    }
    catch (std::invalid_argument)
    {
        printf("[-] Failed to parse thread ID (invalid_argument)\n");
        return STATUS_FAILURE_I;
    }
    catch (std::out_of_range)
    {
        printf("[-] Failed to parse thread ID (out_of_range)\n");
        return STATUS_FAILURE_I;
    }

    // get a handle to the thread    
    // TODO: break this up into separate OpenThread() calls 
    // to avoid unecessary access rights
    HANDLE hThread = ::OpenThread(
        THREAD_QUERY_LIMITED_INFORMATION |     
        THREAD_SET_LIMITED_INFORMATION, 
        FALSE, 
        ThreadId
        );
    if (NULL == hThread)
    {
        printf("[-] Failed to acquire handle to thread (OpenThread())\n");
        printf("[-] GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    // determine if this is a query or a set operation
    BOOL query = argc == 2;


    HRESULT result;

    if (query)
    {
        printf("[+] Querying description for thread %u\n", ThreadId);

        PWSTR description;
        result = ::GetThreadDescription(hThread, &description);
        if (SUCCEEDED(result))
        {
            printf("[+] Got description:\n");
            printf("\t %ws\n", description);

            ::LocalFree(description);
        }
        else
        {
            printf("[-] Failed to query description for thread (GetThreadDesciption())\n");
            // TODO: extract failure information from HRESULT
        }
    }
    else
    {
        printf("[+] Setting description for thread %u\n", ThreadId);

        result = ::SetThreadDescription(hThread, argv[2]);
        if (SUCCEEDED(result))
        {
            printf("[+] Set description\n");
        }
        else
        {
            printf("[-] Failed to set description for thread (SetThreadDescription())\n");
            // TODO: extract failure information from HRESULT
        }
    }

    return SUCCEEDED(result) ? STATUS_SUCCESS_I : STATUS_FAILURE_I;
}