// mmio_write.cpp
//
// Demonstration of using memory-mapped IO to write to a new file.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 mmio_write.cpp

#include <windows.h>

#include <iostream>
#include <string_view>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr unsigned long low_size  = (1 << 10);
constexpr unsigned long HighSize = 0;

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
        argv[1],                // file name
        GENERIC_READ            // desired access
        | GENERIC_WRITE,  
        0,                      // no sharing
        NULL,                   // default security attributes
        CREATE_NEW,             // creation disposition
        FILE_ATTRIBUTE_NORMAL,  // no additional flags
        NULL                    /* no template */);

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
        HighSize,
        low_size,
        NULL
    );

    if (NULL == mapping)
    {
        error("Failed to create file mapping (CreateFileMapping()");
        ::CloseHandle(file);
        return STATUS_FAILURE_I;
    }

    info("Successfully created file mapping");

    auto view = ::MapViewOfFile(
        mapping,            // mapping object handle
        FILE_MAP_WRITE,     // write access 
        0, 0, 0);

    if (NULL == view)
    {
        error("Failed to map view of file (MaviewOfFile())");
        ::CloseHandle(file);
        ::CloseHandle(mapping);
        return STATUS_FAILURE_I;
    }

    info("Successfully mapped view of file");

    auto buffer = static_cast<BYTE*>(view);
    for (auto i = 0u; i < low_size; ++i)
    {
        // fill the mapped region with arbitrary data
        buffer[i] = i % 256;
    }

    ::UnmapViewOfFile(view);
    ::CloseHandle(mapping);
    ::CloseHandle(file);

    return STATUS_SUCCESS_I;
}

