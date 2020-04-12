// system_info.cpp
//
// Using the Win32 API to query various system memory properties.
//
// Compare the results when run from a 64-bit process and a 32-bit (WoW64)
// process on a 64-bit system (i.e. compile once from the x64 native tools
// command prompt and again from the x86 native tools command prompt)
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 system_info.cpp

#include <windows.h>

#include <string>
#include <cstdio>

char const* get_processor_architecture(unsigned short arch)
{
    return [arch]()
    {
        switch (arch)
        {
        case PROCESSOR_ARCHITECTURE_AMD64:
            return "x64";
        case PROCESSOR_ARCHITECTURE_INTEL:
            return "x86";
        case PROCESSOR_ARCHITECTURE_ARM64:
            return "ARM64";
        case PROCESSOR_ARCHITECTURE_ARM:
            return "ARM";
        default:
            return "Unknown";
        }
    }();
}

void display_information(
    SYSTEM_INFO const* info,
    char const*        title)
{
    printf("%s\n%s\n", title, std::string(::strlen(title), '-').c_str());
    printf("%-24s%s\n", "Processor Architecture:",
        get_processor_architecture(info->wProcessorArchitecture));
    printf("%-24s%u\n", "Number of Processors:", info->dwNumberOfProcessors);
    printf("%-24s0x%llX\n", "Active Processor Mask:",
        (unsigned long long)info->dwActiveProcessorMask);
    printf("%-24s%u KB\n", "Page Size:", info->dwPageSize >> 10);
    printf("%-24s0x%p\n", "Min User Space Address:", 
        info->lpMinimumApplicationAddress);
    printf("%-24s0x%p\n", "Max User Space Address:",
        info->lpMaximumApplicationAddress);
    printf("%-24s%u KB\n", "Allocation Granularity:",
        info->dwAllocationGranularity >> 10);
    puts("");
}

int main()
{
    auto info = SYSTEM_INFO{};
    ::GetSystemInfo(&info);
    display_information(&info, "System Information");

    if constexpr (sizeof(void*) == 4)
    {
        auto is_wow64 = FALSE;

        if (::IsWow64Process(::GetCurrentProcess(), &is_wow64) &&
            is_wow64)
        {
            // this is a WoW64 process
            ::GetNativeSystemInfo(&info);
            display_information(&info, "Native System Information");
        }
    }

    return 0;
}