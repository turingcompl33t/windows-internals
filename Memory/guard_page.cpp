// guard_page.cpp
//
// Demo of utilizing virtual memory API to create a guard page.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 guard_page.cpp

#include <windows.h>
#include <cstdio>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

int main()
{
    auto info = SYSTEM_INFO{};
    ::GetSystemInfo(&info);

    auto dwPageSize              = info.dwPageSize;
    auto dwAllocationGranularity = info.dwAllocationGranularity;

    printf("[+] Page Size: %u\n", dwPageSize);
    printf("[+] Allocation Granularity: %u\n", dwAllocationGranularity);

    auto ptr = ::VirtualAlloc(
        nullptr, 
        dwPageSize, 
        MEM_COMMIT | MEM_RESERVE, 
        PAGE_READONLY | PAGE_GUARD);
    if (NULL == ptr)
    {
        printf("[-] Failed to allocate page (VirtualAlloc())\n");
        printf("[-] GLE: %u\n", ::GetLastError());
        return STATUS_FAILURE_I;
    }

    printf("[+] Successfully allocated page\n");

    // TODO: determine why this example did not work with SEH mechanisms;
    // e.g. when I wrapped this VirtualLock() call in an SEH guarded
    // region and had the filter expression test for STATUS_GUARD_PAGE_VIOLATION
    // the handler was never invoked, and the page was successfully locked(?)
    //
    // Working Theory: the exception is swallowed by the virtual memory
    // API and is reflected to the user-level API by the return value
    // of the VirtualLock() call

    // attempt to lock the page
    auto bLocked = ::VirtualLock(ptr, dwPageSize);
    if (!bLocked)
    {
        printf("[+] Failed to lock page on first attempt...\n");
    }
    else
    {
        printf("[+] Successfully locked page on first attempt\n");
        ::VirtualUnlock(ptr, dwPageSize);
    }

    bLocked = ::VirtualLock(ptr, dwPageSize);
    if (!bLocked)
    {
        printf("[+] Failed to lock page on second attempt...\n");
    }
    else
    {
        printf("[+] Successfully locked page on second attempt\n");
        ::VirtualUnlock(ptr, dwPageSize);
    }

    // dwSize must be 0 for use with MEM_RELEASE 
    ::VirtualFree(ptr, 0, MEM_RELEASE);

    return STATUS_SUCCESS_I;
}
