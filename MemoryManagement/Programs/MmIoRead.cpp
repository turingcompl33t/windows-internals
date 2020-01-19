// MmIoRead.cpp
// Demonstration of using memory-mapped IO to read from an existing file.
//
// Build:
//  cl /EHsc /nologo /std:c++17 MmIoRead.cpp

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <string_view>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr DWORD LowSize  = (1 << 10);
constexpr DWORD HighSize = 0;

VOID LogInfo(const std::string_view msg);
VOID LogWarning(const std::string_view msg);
VOID LogError(const std::string_view msg);

INT _tmain(INT argc, PTCHAR argv[])
{
    if (argc != 2)
    {
        LogWarning("Error: invalid arguments");
        LogWarning("Usage: MmIoRead.exe <FILENAME>");
        return STATUS_FAILURE_I;
    }

    HANDLE hFile       = INVALID_HANDLE_VALUE;
    HANDLE hMapping    = NULL;
    LPVOID pView       = NULL;
    LPBYTE pBufferView = NULL;

    auto status = STATUS_SUCCESS_I;

    hFile = ::CreateFileW(
        argv[1],                 // file name
        GENERIC_READ             // desired access
        | GENERIC_WRITE,   
        0,                       // no sharing
        NULL,                    // default security attributes
        OPEN_EXISTING,           // creation disposition
        FILE_ATTRIBUTE_NORMAL,   // no additional flags
        NULL                     // no template
    );

    if (INVALID_HANDLE_VALUE == hFile)
    {
        LogError("Failed to acquire handle to file (CreateFile())");
        status = STATUS_FAILURE_I;
        goto EXIT;
    }

    LogInfo("Successfully acquired handle to file");

    hMapping = ::CreateFileMappingW(
        hFile,
        NULL,
        PAGE_READWRITE,
        0,
        0,
        NULL
    );

    if (NULL == hMapping)
    {
        LogError("Failed to create file mapping (CreateFileMapping()");
        status = STATUS_FAILURE_I;
        goto CLEANUP;
    }

    LogInfo("Successfully created file mapping");

    pView = ::MapViewOfFile(
        hMapping,
        FILE_MAP_READ,
        0, 
        0,
        0
    );

    if (NULL == pView)
    {
        LogError("Failed to map view of file (MapViewOfFile())");
        goto CLEANUP;
    }

    LogInfo("Successfully mapped view of file");

    pBufferView = static_cast<PBYTE>(pView);
    for (UINT i = 0; i < LowSize; ++i)
    {
        // read from the mapped region
        std::cout << pBufferView[i] << ' ';
    }
    std::cout << std::endl;

CLEANUP:
    if (NULL != pView)
    {
        ::UnmapViewOfFile(pView);
    }
    if (INVALID_HANDLE_VALUE != hFile)
    {
        ::CloseHandle(hFile);
    }

    if (NULL != hMapping)
    {
        ::CloseHandle(hMapping);
    }

EXIT:
    return status;
}

VOID LogInfo(const std::string_view msg)
{
    std::cout << "[+] " << msg << std::endl;
}

VOID LogWarning(const std::string_view msg)
{
    std::cout << "[-] " << msg << std::endl;
}

VOID LogError(const std::string_view msg)
{
    std::cout << "[!] " << msg << '\n';
    std::cout << "[!]\tGLE: " << GetLastError() << '\n';
    std::cout << std::endl;
}