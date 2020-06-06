// get_windows_boot_drive.cpp
// 
// Build
//  cl /EHsc /nologo /std:c++17 /W4 get_windows_boot_drive.cpp

#include <windows.h>

#include <memory>
#include <optional>
#include <iostream>
#include <string_view>

constexpr static auto const SUCCESS = 0x0;
constexpr static auto const FAILURE = 0x1;

constexpr static const auto BUFFER_SIZE = 256;

void error(
    std::string_view msg,
    unsigned long const code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << " (" << code << ")\n";
}

[[nodiscard]]
std::optional<std::string> get_windows_physical_drive()
{
    // query the required buffer size
    auto required_len = ::GetSystemDirectoryA(nullptr, 0);
    auto system_dir   = std::make_unique<char[]>(required_len);

    // actually get the system directory
    required_len = ::GetSystemDirectoryA(system_dir.get(), required_len);

    // prepare filename of NTFS volume
    auto ntfs_volume = std::make_unique<char[]>(BUFFER_SIZE);
    SecureZeroMemory(ntfs_volume.get(), BUFFER_SIZE);

    ::_snprintf_s(
        ntfs_volume.get(), 
        BUFFER_SIZE,
        BUFFER_SIZE-1, 
        "\\\\.\\%c%c", 
        system_dir.get()[0], 
        system_dir.get()[1]);

    // acquire a handle to the NTFS volume
    auto volume_handle = ::CreateFileA(
        ntfs_volume.get(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (INVALID_HANDLE_VALUE == volume_handle)
    {
        error("Failed to acquire handle to NTFS volume");
        return std::nullopt;
    }

    // use the handle to the volume to query drive extents
    auto disk_extents = std::make_unique<char[]>(BUFFER_SIZE);
    auto bytes_out = unsigned long{};
    auto const res = ::DeviceIoControl(
        volume_handle,
        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
        nullptr,
        0,
        disk_extents.get(),
        BUFFER_SIZE,
        &bytes_out,
        nullptr);
    if (!res)
    {
        error("Failed to query volume disk extents");
        return std::nullopt;
    }

    // get the disk number from first returned disk extents structure
    auto& extents = *reinterpret_cast<VOLUME_DISK_EXTENTS*>(disk_extents.get());
    auto const disk_number = extents.Extents[0].DiskNumber;

    auto physical_drive = std::make_unique<char[]>(BUFFER_SIZE);
    SecureZeroMemory(physical_drive.get(), BUFFER_SIZE);

    ::_snprintf_s(
        physical_drive.get(), 
        BUFFER_SIZE, 
        BUFFER_SIZE-1, 
        "\\\\.\\PhysicalDrive%u", 
        disk_number);

    auto const physical_drive_name = std::string(
        physical_drive.get(), 
        strnlen_s(physical_drive.get(), BUFFER_SIZE));

    return physical_drive_name;
}

int main()
{
    auto physical_drive_opt = get_windows_physical_drive();
    if (!physical_drive_opt.has_value())
    {
        return FAILURE;
    }

    auto& physical_drive_name = physical_drive_opt.value();
    std::cout << "[+] Name of physical drive hosting current instance of Windows is: "
        << physical_drive_name << '\n';

    return SUCCESS;
}