// enumerate_native.cpp
//
// Utilizing the native API to enumerate processes.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 enumerate_native.cpp

#include <windows.h>
#include <winternl.h>

#include <vector>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <string_view>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr auto STATUS_INFO_LENGTH_MISMATCH = 0xC0000004;

// NOTE: the native API function NtQuerySystemInformation()
// with the SystemProcessInformation class provides so much
// more information than what is presented here; this is more
// of a POC than anything else - untapped potential.

struct Process
{
    std::wstring  image_name;
    UINT_PTR      process_id;
    unsigned long n_threads;
    unsigned long handle_count;

    friend std::wostream& operator<<(std::wostream& os, Process const& p)
    {
        os << "PID: " << p.process_id << " "
           << "Image: " << p.image_name << '\n'
           << "\tThread Count: " << p.n_threads << '\n'
           << "\tHandle Count: " << p.handle_count << '\n';

        return os;
    }
};

using query_sys_info_f = NTSTATUS (__stdcall*)(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID                   SystemInformation,
    IN ULONG                    SystemInformationLength,
    OUT PULONG                  ReturnLength
    );

void trace_nterror(
    std::string_view msg,
    long code
    )
{
    std::cout << std::hex
        << "[-] " << msg
        << " Code: " << code
        << std::dec 
        << std::endl;
}

void trace_error(
    std::string_view msg, 
    unsigned long code = ::GetLastError()
    )
{
    std::cout << std::hex
        << "[-] " << msg
        << " Code: " << code
        << std::dec 
        << std::endl;
}

void trace_info(std::string_view msg)
{
    std::cout << "[+] " << msg << std::endl;
}

unsigned long round_to_nearest_power2(unsigned long l)
{
    l--;
    l |= l >> 1;
    l |= l >> 2;
    l |= l >> 4;
    l |= l >> 8;
    l |= l >> 16;
    return ++l;
}

std::vector<Process> parse_results(std::unique_ptr<char[]>& buffer)
{
    auto parsed = std::vector<Process>{};

    auto cursor  = buffer.get();
    auto current = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(cursor);
    while (current->NextEntryOffset != 0)
    {
        auto p = Process{};

        // UNICODE_STRING specifies length in bytes
        // std::wstring expects character count for construction
        p.image_name = std::wstring(
                current->ImageName.Buffer, 
                (current->ImageName.Length/sizeof(wchar_t))
                );

        p.process_id   = reinterpret_cast<UINT_PTR>(current->UniqueProcessId);
        p.handle_count = current->HandleCount;
        p.n_threads    = current->NumberOfThreads;

        parsed.push_back(std::move(p));

        // seek to the next entry
        cursor += current->NextEntryOffset;
        current = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(cursor);
    }

    return parsed;
}

int main()
{
    // get the address of function exported by native API
    auto query_sys_info = 
        reinterpret_cast<query_sys_info_f>(
            ::GetProcAddress(::GetModuleHandleA("ntdll"), "NtQuerySystemInformation"));

    if (NULL == query_sys_info)
    {
        trace_error("GetProcAddress() failed");
        return STATUS_FAILURE_I;
    }

    trace_info("Resolved NtQuerySystemInformation()");

    auto required_length = unsigned long{};

    // query the required buffer size
    auto status = query_sys_info(SystemProcessInformation, nullptr, 0, &required_length);
    if (!NT_SUCCESS(status) &&
        status != STATUS_INFO_LENGTH_MISMATCH)
    {
        trace_nterror("NtQuerySystemInformation() failed", status);
        return STATUS_FAILURE_I;
    }

    auto buffer_size = round_to_nearest_power2(required_length);

    // allocate a buffer large enough to hold results
    auto buffer = std::make_unique<char[]>(buffer_size);

    // attempt the call for real
    status = query_sys_info(
        SystemProcessInformation, 
        buffer.get(), 
        buffer_size, 
        &required_length
        );

    if (!NT_SUCCESS(status))
    {
        trace_nterror("NtQuerySystemInformation() failed", status);
        return STATUS_FAILURE_I;
    }

    trace_info("NtQuerySystemInformation() succeeded");

    auto parsed = parse_results(buffer);

    for (auto const& p : parsed)
    {
        std::wcout << p;
    }
    std::wcout << std::flush;

    return STATUS_SUCCESS_I;
}