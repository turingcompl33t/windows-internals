// get_partition_info.cpp
//
// NOTE: requires elevation
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 get_partition_info.cpp

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

[[nodiscard]]
std::optional<PARTITION_INFORMATION_EX>
get_disk_partition_info(std::string const& drive_name)
{
    auto partition_info = PARTITION_INFORMATION_EX{};

    auto const drive_handle = ::CreateFileA(
        drive_name.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (INVALID_HANDLE_VALUE == drive_handle)
    {
        error("Failed to acquire handle to drive");
        return std::nullopt;
    }

    auto bytes_out = unsigned long{};
    auto const ret = ::DeviceIoControl(
        drive_handle,
        IOCTL_DISK_GET_PARTITION_INFO_EX,
        nullptr,
        0,
        &partition_info,
        sizeof(partition_info),
        &bytes_out,
        nullptr);
    if (!ret)
    {
        error("Failed to query drive partition information");
        ::CloseHandle(drive_handle);
        return std::nullopt;
    }

    ::CloseHandle(drive_handle);
    return partition_info;
}

std::string partition_style_string(PARTITION_STYLE style)
{
    return [=](){
        switch(style)
        {
        case PARTITION_STYLE_MBR:
            return "MBR";
        case PARTITION_STYLE_GPT:
            return "GPT";
        case PARTITION_STYLE_RAW:
            return "RAW";
        default:
            return "UNRECOGNIZED";
        }
    }();
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

    auto const partition_info_opt = get_disk_partition_info(physical_drive_name);
    if (!partition_info_opt.has_value())
    {
        return FAILURE;
    }

    auto& partition_info = partition_info_opt.value();
    
    std::cout << "[+] Disk Partition Style: " 
        << partition_style_string(partition_info.PartitionStyle) << '\n';
    std::cout << "[+] Partition Length: " 
        << partition_info.PartitionLength.QuadPart << '\n';
    std::cout << "[+] Partition Starting Offset: " 
        << partition_info.StartingOffset.QuadPart << '\n';

    if (PARTITION_STYLE_MBR == partition_info.PartitionStyle)
    {
        auto& mbr = partition_info.Mbr;
        std::cout << "[+] MBR Partition Information:\n"
            << "\tBoot Indicator: " << static_cast<unsigned>(mbr.BootIndicator) << '\n'
            << "\tRecognized Partition: " << static_cast<unsigned>(mbr.RecognizedPartition) << '\n'
            << "\tHidden Sectors: " << mbr.HiddenSectors << '\n';
    }
    else if (PARTITION_STYLE_GPT == partition_info.PartitionStyle)
    {
        // TODO
        // auto& gpt = partition_info.Gpt;
        std::cout << "[+] GPT Partition Information:\n";
    }

    return SUCCESS;
}