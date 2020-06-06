// enumerate_drives.cpp
//
// NOTE: requires elevation
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 enumerate_drives.cpp

#include <windows.h>

#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <iostream>
#include <string_view>

constexpr static auto const SUCCESS = 0x0;
constexpr static auto const FAILURE = 0x1;

void error(
    std::string_view msg,
    unsigned long const code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg << " (" << code << ")"
        << std::endl; 
}

std::optional<std::vector<char>> 
get_logical_drives()
{
    auto const mask = ::GetLogicalDrives();
    if (0 == mask)
    {
        error("GetLogicalDrives() failed");
        return std::nullopt;
    }

    auto v = std::vector<char>{};
    for (auto bit_pos = 1; bit_pos < sizeof(DWORD)*CHAR_BIT; ++bit_pos)
    {
        DWORD const drive_id = 1 << bit_pos;
        if (drive_id & mask)
        {
            auto const c = static_cast<char>(bit_pos + 'A');
            v.push_back(c);
        }
    }
    
    return v;
}

char const* drive_type_to_string(UINT const drive_type)
{
    return [=]()
    {
        switch (drive_type)
        {
            case DRIVE_UNKNOWN:
                return "DRIVE_UNKNOWN";
            case DRIVE_NO_ROOT_DIR:
                return "DRIVE_NO_ROOT_DIR";
            case DRIVE_REMOVABLE:
                return "DRIVE_REMOVABLE";
            case DRIVE_FIXED:
                return "DRIVE_FIXED";
            case DRIVE_REMOTE:
                return "DRIVE_REMOTE";
            case DRIVE_CDROM:
                return "DRIVE_CDROM";
            case DRIVE_RAMDISK:
                return "DRIVE_RAMDISK";
            default:
                return "UNRECOGNIZED";
        }
    }();
}

void get_drive_type(char const drive_letter)
{
    char buffer[MAX_PATH];
    SecureZeroMemory(buffer, _countof(buffer));

    _snprintf_s(buffer, MAX_PATH-1, "\\\\.\\%c:\\", drive_letter);

    auto const type = ::GetDriveTypeA(buffer);

    std::cout << "[+] Drive type for logical drive " 
        << drive_letter << ": " 
        << drive_type_to_string(type) << '\n';
}

std::unique_ptr<char[]> 
query_volume_disk_extents(HANDLE volume_handle)
{
    auto bufsz  = sizeof(VOLUME_DISK_EXTENTS);
    auto buffer = std::make_unique<char[]>(bufsz);
    
    auto bytes_out = unsigned long{};
    for (;;)
    {
        auto res = ::DeviceIoControl(
            volume_handle,
            IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
            nullptr,
            0,
            buffer.get(),
            MAX_PATH,
            &bytes_out,
            nullptr);

        if (!res)
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                bufsz += sizeof(DISK_EXTENT);
                buffer = std::make_unique<char[]>(bufsz);
                continue;
            }
            else
            {
                // fail
                error("Failed to query volume disk extents");
                return nullptr;
            }
        }

        break;
    }

    return buffer;
}

void get_disks_for_logical_drive(char const drive_letter)
{
    char drive_name[MAX_PATH];
    SecureZeroMemory(drive_name, _countof(drive_name));

    // construct the complete drive name
    ::_snprintf_s(drive_name, MAX_PATH-1, "\\\\.\\%c:", drive_letter);

    auto drive_handle = ::CreateFileA(
        drive_name,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (INVALID_HANDLE_VALUE == drive_handle)
    {
        error("Failed to acquire handle to logical drive");
        return;
    }

    auto disk_extents_buffer = query_volume_disk_extents(drive_handle);
    if (!disk_extents_buffer)
    {
        return;
    }

    auto& disk_extents = *reinterpret_cast<VOLUME_DISK_EXTENTS*>(disk_extents_buffer.get());

    std::cout << "[+] Disk information for logical drive " << drive_letter << ":\n"
        << "\t[+] Number of disks spanned by volume: " << disk_extents.NumberOfDiskExtents << '\n';

    for (auto i = 0u; i < disk_extents.NumberOfDiskExtents; ++i)
    {
        auto& extent = disk_extents.Extents[i];
        std::cout << "\t[+] Disk number: " << extent.DiskNumber << '\n'
            << "\t[+] Starting offset: " << extent.StartingOffset.QuadPart << '\n'
            << "\t[+] Extent length: " << extent.ExtentLength.QuadPart << '\n';
    }
}

int main()
{
    auto logical_drives_opt = get_logical_drives();
    if (!logical_drives_opt.has_value() 
        || logical_drives_opt.value().size() == 0)
    {
        error("Failed to enumerate logical drives", 0);
        return FAILURE;
    }

    std::cout << "[+] Enumerated available logical drives\n";

    auto& logical_drives = logical_drives_opt.value();

    for (auto const drive_letter : logical_drives)
    {
        get_drive_type(drive_letter);
    }

    for (auto const drive_letter : logical_drives)
    {
        get_disks_for_logical_drive(drive_letter);
    }

    return SUCCESS;
}