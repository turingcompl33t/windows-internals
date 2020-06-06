// read_mbr.cpp
//
// NOTE: requires elevation
// NOTE: assumes drive is MBR-formatted
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 read_mbr.cpp

#include <windows.h>

#include <cstdio>
#include <string>
#include <memory>
#include <optional>
#include <iostream>
#include <string_view>

constexpr static auto const SUCCESS = 0x0;
constexpr static auto const FAILURE = 0x1;

#pragma pack(push, 1)
struct MBR_PARTITION_TABLE_ENTRY
{
    BYTE  status;
    BYTE  chs_first[3];
    BYTE  type;
    BYTE  chs_last[3];
    DWORD lba_first;
    DWORD size;
};
#pragma pack(pop)

struct MASTER_BOOT_RECORD 
{
    BYTE                      bootcode[0x1BE];
    MBR_PARTITION_TABLE_ENTRY partition_table[4];
    WORD                      signature;
};

void error(
    std::string_view msg,
    unsigned long const code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << " (" << code << ")\n";
}

std::optional<unsigned long> 
parse_ul(std::string_view str)
{
    try
    {
        auto const v = std::stoul(str.data());
        return v;
    }
    catch (std::exception const&)
    {
        return std::nullopt;
    }
}

size_t dump_n_bytes(void const* buffer, size_t n)
{
    auto* view = reinterpret_cast<unsigned char const*>(buffer);
    for (auto i = 0u; i < n; ++i)
    {
        printf("%02X ", view[i]);
    }

    puts("");

    return n;
}

void dump_bytes(void const* buffer, size_t count, size_t columns)
{
    auto* view = reinterpret_cast<char const*>(buffer);

    auto dumped = size_t{};
    while (dumped < count)
    {
        auto const to_dump = min(columns, count - dumped);
        dumped += dump_n_bytes(&view[dumped], to_dump);
    }
}

bool read_at_offset(
    HANDLE   file,
    void*    buffer,
    size_t   buffer_size,
    uint64_t offset
    )
{
    auto tmp = LARGE_INTEGER{};
    tmp.QuadPart = offset;
    if (!::SetFilePointerEx(file, tmp, nullptr, FILE_BEGIN))
    {
        error("SetFilePointerEx() failed");
        return false;
    }

    auto bytes_read = unsigned long{};
    auto const success = ::ReadFile(
        file,
        buffer,
        static_cast<unsigned long>(buffer_size),  // narrow
        &bytes_read,
        nullptr);

    if (!success)
    {
        error("ReadFile() failed");
    }

    return success;
}

HANDLE open_drive_handle(std::string_view drive_name)
{
    auto const handle = ::CreateFileA(
        drive_name.data(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    return handle;
}

bool validate_mbr(MASTER_BOOT_RECORD const& mbr)
{
    return mbr.signature == 0xAA55;
}

void dump_partition_info(MBR_PARTITION_TABLE_ENTRY& entry)
{
    std::cout << "[+] Partition table entry:\n"
        << "\t[+] Partition type: " << static_cast<unsigned>(entry.type) << '\n'
        << "\t[+] Start LBA: " << entry.lba_first << '\n'
        << "\t[+] Partition size: " << entry.size << '\n';
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "[-] Invalid arguments\n"
            << "[-] Usage: " << argv[0] << " <DISK NUMBER>\n";
        return FAILURE;
    }   

    auto drive_number_opt = parse_ul(argv[1]);
    if (!drive_number_opt.has_value())
    {
        error("Unable to parse drive number");
        return FAILURE;
    }

    auto& drive_number = drive_number_opt.value();

    char drive_name[MAX_PATH];
    SecureZeroMemory(drive_name, _countof(drive_name));
    _snprintf_s(drive_name, MAX_PATH-1, "\\\\.\\PhysicalDrive%u", drive_number);

    std::cout << "[+] Acquiring handle for drive: "
        << drive_name << '\n';

    auto drive = open_drive_handle(drive_name);
    if (INVALID_HANDLE_VALUE == drive)
    {
        error("Failed to acquire handle to drive");
        return FAILURE;
    }

    std::cout << "[+] Successfully acquired handle to drive\n"
        << "[+] Reading MBR...\n";

    auto mbr = MASTER_BOOT_RECORD{};
    if (!read_at_offset(drive, &mbr, sizeof(mbr), 0))
    {
        error("Failed to read MBR", 0);
        ::CloseHandle(drive);
        return FAILURE;
    }

    std::cout << "[+] Successfully read drive MBR\n";
    // dump_bytes(&mbr, sizeof(MASTER_BOOT_RECORD), 16);

    if (!validate_mbr(mbr))
    {
        error("MBR invalid or corrupt");
        ::CloseHandle(drive);
        return FAILURE;
    }

    std::cout << "[+] MBR is valid\n";
    for (auto i = 0u; i < 4; ++i)
    {
        auto& partition_entry = mbr.partition_table[i];
        dump_partition_info(partition_entry);
    }

    ::CloseHandle(drive);
    return SUCCESS;
}