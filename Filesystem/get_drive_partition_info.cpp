// get_drive_partition_info.cpp
//
// NOTE: requires elevation
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 get_drive_partition_info.cpp

#include <windows.h>

#include <string>
#include <memory>
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
    std::cout << "[-] " << msg
        << " (" << code << ")\n";
}

[[nodiscard]]
std::optional<PARTITION_INFORMATION_EX>
get_disk_partition_info(std::string_view drive_name)
{
    auto partition_info = PARTITION_INFORMATION_EX{};

    auto const drive_handle = ::CreateFileA(
        drive_name.data(),
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

// ripped off from StackOverflow
// https://stackoverflow.com/questions/1672677/print-a-guid-variable
std::ostream& operator<<(std::ostream& os, REFGUID guid){

    os << std::uppercase;
    os.width(8);
    os << std::hex << guid.Data1 << '-';

    os.width(4);
    os << std::hex << guid.Data2 << '-';

    os.width(4);
    os << std::hex << guid.Data3 << '-';

    os.width(2);
    os << std::hex
        << static_cast<short>(guid.Data4[0])
        << static_cast<short>(guid.Data4[1])
        << '-'
        << static_cast<short>(guid.Data4[2])
        << static_cast<short>(guid.Data4[3])
        << static_cast<short>(guid.Data4[4])
        << static_cast<short>(guid.Data4[5])
        << static_cast<short>(guid.Data4[6])
        << static_cast<short>(guid.Data4[7]);
    os << std::nouppercase;
    return os;
}

std::optional<unsigned long> parse_ul(std::string_view str)
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

    std::cout << "[+] Querying partition information for drive: "
        << drive_name << '\n';

    auto const partition_info_opt = get_disk_partition_info(drive_name);
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
        auto& gpt = partition_info.Gpt;
        std::cout << "[+] GPT Partition Information:\n"
            << "\t[+] Partition type: " << gpt.PartitionType << '\n'
            << "\t[+] Partition ID: " << gpt.PartitionId << '\n';
    }

    return SUCCESS;
}