// IoCompletionPort.cpp
// Demonstration of IO completion port usage.
//
// Build:
//  cl /EHsc /nologo /std:c++17 IoCompletionPort.cpp

#include <windows.h>
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

DWORD GetNumberOfProcessors();

INT wmain(INT argc, WCHAR* argv[])
{
    if (argc != 2)
    {
        printf("[-] Error: invalid arguments\n");
        printf("[-] Usage: %ws <ROOT DIR>\n", argv[0]);
        return STATUS_FAILURE_I;
    }

    if (!fs::is_directory(argv[1]))
    {
        printf("[-] Invalid path to root directory specified\n");
        return STATUS_FAILURE_I;
    }

    for (auto& p : fs::recursive_directory_iterator{argv[1]})
    {
        // process only regular files
        if (!p.is_regular_file()) continue;

        OVERLAPPED ov;
        //::CreateFileW()
    }

    auto nProcessors = GetNumberOfProcessors();

    return STATUS_SUCCESS_I;
}

DWORD GetNumberOfProcessors()
{
    SYSTEM_INFO info{};
    ::GetSystemInfo(&info);   
    
    return info.dwNumberOfProcessors;
}

