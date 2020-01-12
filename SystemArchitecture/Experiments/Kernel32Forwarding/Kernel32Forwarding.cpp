// Kernel32Forwarding.cpp
// Exploring Kernel32 export forwarding and trampolines.
//
// Compile:
//  cl /EHsc /nologo Kernel32Forwarding.cpp

#include <windows.h>
#include <iostream>

int main()
{   
    HANDLE hFile;

    __debugbreak();

    hFile = ::CreateFileW(
        L"C:\\Dev\\WinInternals\\Experiments\\Kernel32Forwarding\\OpenMe.txt",
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (INVALID_HANDLE_VALUE == hFile)
    {
        std::cout << "CreateFileW() failed\n";
        std::cout << "GLE: " << GetLastError() << '\n';
        return 1;
    }

    ::CloseHandle(hFile);

    return 0;
}