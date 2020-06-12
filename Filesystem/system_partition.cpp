// system_partition.cpp
//
// Build
//   cl /EHsc /nologo /std:c++17 /W4 system_partition.cpp

#include <windows.h>
#include <winternl.h>

#include <cstdio>
#include <string>
#include <vector>
#include <optional>
#include <string_view>

constexpr static auto const SUCCESS = 0x0;
constexpr static auto const FAILURE = 0x1;

// SYSTEM_INFORMATION_CLASS
constexpr static auto const QUERY = 0x62;

using nt_query_system_information_f = NTSTATUS (__stdcall*)(
    SYSTEM_INFORMATION_CLASS SystemInformationClass,
    PVOID                    SystemInformation,
    ULONG                    SystemInformationLength,
    PULONG                   ReturnLength
    );

struct system_partition_information
{
    int64_t reserved1;
    int64_t reserved2;
    wchar_t system_partition[1];
};

void error(
    std::string_view msg,
    unsigned long const code = ::GetLastError()
    )
{
    printf("[-] %s (%u)\n", msg.data(), code);
}

void trace(std::string_view msg)
{
    printf("[+] %s\n", msg.data());
}

[[nodiscard]]
std::optional<std::wstring> 
get_system_partition_name()
{
    auto* nt_query_system_information 
        = reinterpret_cast<nt_query_system_information_f>(
            ::GetProcAddress(::GetModuleHandleW(L"ntdll"), "NtQuerySystemInformation"));
    if (nullptr == nt_query_system_information)
    {
        error("Failed to resolve NtQuerySystemInformation()");
        return std::nullopt;
    }

    char buffer[MAX_PATH];

    auto bytes_out = unsigned long{};
    auto const status = nt_query_system_information(
        static_cast<SYSTEM_INFORMATION_CLASS>(QUERY),
        buffer,
        _countof(buffer),
        &bytes_out);

    if (!NT_SUCCESS(status))
    {
        error("NtQuerySystemInformation() failed", status);
        return std::nullopt;
    }


    auto& partition_info   = *reinterpret_cast<system_partition_information*>(buffer);
    auto* system_partition = partition_info.system_partition;

    return std::make_optional(
        std::wstring{system_partition, 
            ::wcsnlen_s(system_partition, MAX_PATH)});
} 

[[nodiscard]]
std::optional<char> get_available_logical_drive()
{
    char query_buffer[MAX_PATH];
    char target_buffer[MAX_PATH];  // buffer not large enough for some queries

    SecureZeroMemory(query_buffer, _countof(query_buffer));
    SecureZeroMemory(target_buffer, _countof(target_buffer));

    for (auto drive_letter = 'A'; drive_letter < 'Z'; ++drive_letter)
    {
        ::_snprintf_s(
            query_buffer, 
            _countof(query_buffer)-1, 
            "%c:", drive_letter);

        auto const result = ::QueryDosDeviceA(
            query_buffer,
            target_buffer,
            _countof(target_buffer));

        if (0 == result && ::GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            printf("[+] Found available drive letter: %c\n", drive_letter);
            return drive_letter;
        }
    }

    return std::nullopt;
}

[[nodiscard]]
std::optional<HANDLE> 
mount_system_partition(std::wstring const& partition_name, char const drive_letter)
{
    wchar_t device_name[MAX_PATH];
    SecureZeroMemory(device_name, _countof(device_name));
    _snwprintf_s(device_name, _countof(device_name)-1, L"%c:", drive_letter);

    auto const result = ::DefineDosDeviceW(
        DDD_RAW_TARGET_PATH,
        device_name,
        partition_name.c_str());

    if (0 == result)
    {
        error("DefineDosDevice() failed");
        return std::nullopt;
    }

    trace("Successfully mounted system partition");

    auto volume = ::CreateFileW(
        device_name,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (INVALID_HANDLE_VALUE == volume)
    {
        error("Failed to acquire handle to mounted system partition");
        return std::nullopt;
    }

    trace("Successfully acquired handle to mounted system partition");

    return volume;
}

void unmount_system_partition(char const drive_letter)
{   
    char buffer[MAX_PATH];
    SecureZeroMemory(buffer, _countof(buffer));
    ::_snprintf_s(buffer, _countof(buffer)-1, "%c:", drive_letter);

    ::DeleteVolumeMountPointA(buffer);
}

int main()
{
    auto system_partition_name = get_system_partition_name();
    if (!system_partition_name.has_value())
    {
        return FAILURE;
    }

    printf("[+] System Partition: %ws\n", system_partition_name->c_str());

    auto const drive_letter = get_available_logical_drive();
    if (!drive_letter.has_value())
    {
        return FAILURE;
    }

    auto volume = mount_system_partition(
        system_partition_name.value(), 
        drive_letter.value());
    if (!volume.has_value())
    {
        return FAILURE;
    }

    getc(stdin);

    unmount_system_partition(drive_letter.value());
    ::CloseHandle(volume.value());

    return SUCCESS;
}