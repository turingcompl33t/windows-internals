// file_size.cpp
//
// Demo of various methods to determine file size.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 file_size.cpp

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
    std::cout << "[-] " << msg << " (" << code << ")"
        << std::endl; 
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "[-] Invalid arguments\n";
        std::cout << "[-] Usage: " << argv[0] << " <FILE PATH>\n";
        return FAILURE;
    }

    auto const* const filename = argv[1];

    auto file = CreateFileA(
        filename,
        GENERIC_READ | 
        FILE_READ_ATTRIBUTES,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (INVALID_HANDLE_VALUE == file)
    {
        error("CreateFile() failed");
        return FAILURE;
    }

    // obtain file size via SetFilePointerEx()
    auto size1 = LARGE_INTEGER{};
    auto zero  = LARGE_INTEGER{ 0 };    
    if (!SetFilePointerEx(file, zero, &size1, FILE_END))
    {
        error("SetFilePointerEx() failed");
    }
    else
    {
        std::cout << "[+] File size obtained via SetFilePointerEx(): " 
            << size1.QuadPart << '\n';
    }

    // obtain file size via GetFileSizeEx()
    auto size2 = LARGE_INTEGER{};
    if (!GetFileSizeEx(file, &size2))
    {
        error("GetFileSizeEx() failed");
    }
    else
    {
        std::cout << "[+] File size obtained via GetFileSizeEx(): " 
            << size2.QuadPart << '\n';
    }

    ::CloseHandle(file);

    return SUCCESS;
}