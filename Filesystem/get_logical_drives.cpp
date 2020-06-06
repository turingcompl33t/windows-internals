// get_logical_drives.cpp
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 get_logical_drives.cpp

#include <windows.h>

#include <iostream>
#include <string_view>

constexpr static auto const SUCCESS = 0x0;
constexpr static auto const FAILURE = 0x1;

void error(
    std::string_view msg,
    unsigned long const code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg << " (" << code << ")\n";
    std::cout << std::flush;
}

int main()
{
    auto const mask = ::GetLogicalDrives();
    if (0 == mask)
    {
        error("GetLogicalDrives() failed");
        return FAILURE;
    }

    for (auto bit_pos = 1; bit_pos < sizeof(DWORD)*CHAR_BIT; ++bit_pos)
    {
        DWORD const drive_id = 1 << bit_pos;
        if (drive_id & mask)
        {
            auto const c = static_cast<char>(bit_pos + 'A');
            std::cout << "[+] Drive " << c << ": available\n";
        }
    }

    return SUCCESS;
}