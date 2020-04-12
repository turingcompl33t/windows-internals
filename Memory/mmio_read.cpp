// mmio_read.cpp
//
// Demonstration of using memory-mapped IO to read from an existing file.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 mmio_read.cpp

#include <windows.h>
#include <iostream>
#include <string_view>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const unsigned long low_size  = (1 << 10);
constexpr const unsigned long high_size = 0;

void info(std::string_view msg)
{
    std::cout << "[+] " << msg << std::endl;
}

void warning(std::string_view msg)
{
    std::cout << "[-] " << msg << std::endl;
}

void error(std::string_view msg)
{
    std::cout << "[!] " << msg << '\n';
    std::cout << "[!]\tGLE: " << GetLastError() << '\n';
    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "[-] Error: invalid arguments\n";
        std::cout << "[-] Usage: " << argv[0] << " <FILENAME>\n";
        return STATUS_FAILURE_I;
    }

    auto file = ::CreateFileA(
        argv[1],                 // file name
        GENERIC_READ             // desired access
        | GENERIC_WRITE,   
        0,                       // no sharing
        NULL,                    // default security attributes
        OPEN_EXISTING,           // creation disposition
        FILE_ATTRIBUTE_NORMAL,   // no additional flags
        NULL                     /* no template */ );

    if (INVALID_HANDLE_VALUE == file)
    {
        error("Failed to acquire handle to file (CreateFile())");
        return STATUS_FAILURE_I;
    }

    info("Successfully acquired handle to file");

    auto mapping = ::CreateFileMappingW(
        file,
        NULL,
        PAGE_READWRITE,
        0,
        0,
        NULL);

    if (NULL == mapping)
    {
        error("Failed to create file mapping (CreateFileMapping()");
        ::CloseHandle(file);
        return STATUS_FAILURE_I;
    }

    info("Successfully created file mapping");

    auto view = ::MapViewOfFile(
        mapping,
        FILE_MAP_READ,
        0, 0, 0);

    if (NULL == view)
    {
        error("Failed to map view of file (MaviewOfFile())");
        ::CloseHandle(file);
        ::CloseHandle(mapping);
        return STATUS_FAILURE_I;
    }

    info("Successfully mapped view of file");

    auto buffer = static_cast<PBYTE>(view);
    for (UINT i = 0; i < low_size; ++i)
    {
        // read from the mapped region
        std::cout << buffer[i] << ' ';
    }
    std::cout << std::endl;

    ::UnmapViewOfFile(view);
    ::CloseHandle(mapping);
    ::CloseHandle(file);
}