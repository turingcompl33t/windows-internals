// FileSize.cpp
// Demo of various methods to determine file size.

#define UNICODE
#define _UNICODE

#include <windows.h>

#include <cstdio>

INT wmain(INT argc, PWCHAR argv[])
{
    if (argc != 2)
    {
        printf("[-] Invalid arguments\n");
        printf("[-] Usage: %ws <FILE PATH>\n", argv[0]);
        return 1;
    }

    const auto filename = argv[1];

    HANDLE hFile = CreateFile(
        filename,
        GENERIC_READ | FILE_READ_ATTRIBUTES,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (INVALID_HANDLE_VALUE == hFile)
    {
        printf("[-] Unable to acquire handle to file (CreateFile())\n");
        printf("GLE: %u\n", GetLastError());
        return 1;
    }

    printf("[+] Successfully acquired handle to file %ws\n", filename);

    // obtain file size via SetFilePointerEx()
    LARGE_INTEGER FileSize1;
    LARGE_INTEGER ZeroOffset;
    ZeroOffset.QuadPart = 0;
    if (!SetFilePointerEx(hFile, ZeroOffset, &FileSize1, FILE_END))
    {
        printf("[-] Failed to obtain file size (SetFilePointerEx())\n");
        printf("[-] GLE: %u\n", GetLastError());
    }
    else
    {
        printf("[+] File size obtained via SetFilePointerEx(): %llu\n", FileSize1.QuadPart);
    }

    // obtain file size via GetFileSizeEx()
    LARGE_INTEGER FileSize2;
    if (!GetFileSizeEx(hFile, &FileSize2))
    {
        printf("[-] Failed to obtain fle size (GetFileSizeEx())\n");
        printf("[-] GLE: %u\n", GetLastError());
    }
    else
    {
        printf("[+] File size obtained via GetFileSizeEx(): %llu\n", FileSize2.QuadPart);
    }

    CloseHandle(hFile);

    return 0;
}