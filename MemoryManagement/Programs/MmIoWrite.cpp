// MmIoWrite.cpp
// Demonstration of using memory-mapped IO to write to a new file.
//
// Build:
//  cl /EHsc /nologo /std:c++17 MmIoWrite.cpp

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
        argv[1],                // file name
        GENERIC_READ            // desired access
        | GENERIC_WRITE,  
        0,                      // no sharing
        NULL,                   // default security attributes
        CREATE_NEW,             // creation disposition
        FILE_ATTRIBUTE_NORMAL,  // no additional flags
        NULL                    // no template
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
        HighSize,
        LowSize,
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
        hMapping,            // mapping object handle
        FILE_MAP_WRITE,      // write access 
        0,                   // offset high
        0,                   // offset low
        0                    // map the entire file
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
        // fill the mapped region with arbitrary data
        pBufferView[i] = i % 256;
    }

CLEANUP:
    if (NULL != pView)
    {
        ::UnmapViewOfFile(pView);
    }

    if (NULL != hMapping)
    {
        ::CloseHandle(hMapping);
    }

    if (INVALID_HANDLE_VALUE != hFile)
    {
        ::CloseHandle(hFile);
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