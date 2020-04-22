// enumerate_native2.cpp
//
// Another native API for process (and thread) enumeration.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 enumerate_native2.cpp

#include <windows.h>
#include <Psapi.h>             // GetModuleFileNameEx
#pragma comment(lib, "psapi")

#include <iostream>
#include <string_view>

constexpr auto const STATUS_SUCCESS_I = 0x0;
constexpr auto const STATUS_FAILURE_I = 0x1;

// NtGetNextProcess()
using nt_get_next_process_f = NTSTATUS(__stdcall*)(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _In_ ULONG Flags,
    _Out_ PHANDLE NewProcessHandle
    );

// NtGetNextThread()
using nt_get_next_thread_f = NTSTATUS(__stdcall*)(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _In_ ULONG Flags,
    _Out_ PHANDLE NewThreadHandle
    );

void error(
    std::string_view msg,
    unsigned long const code = ::GetLastError()
    )
{
    std::cout << "[+] " << msg
        << " Code: " << std::hex << code << std::dec
        << std::endl;
}

int main()
{
    auto ntdll = ::GetModuleHandleW(L"ntdll");
    if (NULL == ntdll)
    {
        error("GetModuleHandle() failed");
        return STATUS_FAILURE_I;
    }

    auto next_process = reinterpret_cast<nt_get_next_process_f>(
        ::GetProcAddress(ntdll, "NtGetNextProcess"));
    auto next_thread = reinterpret_cast<nt_get_next_thread_f>(
        ::GetProcAddress(ntdll, "NtGetNextThread"));
    if (NULL == next_process || NULL == next_thread)
    {
        error("Failed to resolve native API call(s)\n");
        return STATUS_FAILURE_I;
    }

    wchar_t exe_buffer[MAX_PATH];

    auto process = HANDLE{nullptr};
    auto thread  = HANDLE{nullptr};
    while (next_process(process, MAXIMUM_ALLOWED, 0, 0, &process) == 0)
    {
        // query the executable image for the process
        ::GetModuleFileNameExW(process, 0, exe_buffer, MAX_PATH);

        std::wcout << L"PID: " << ::GetProcessId(process)
            << L" Commandline: " << exe_buffer << L'\n';

        while (next_thread(process, thread, MAXIMUM_ALLOWED, 0, 0, &thread) == 0)
        {
            std::cout << "\tTID: " << ::GetThreadId(thread) << '\n';
        }
        std::cout << '\n';
    }

    return STATUS_SUCCESS_I;
}