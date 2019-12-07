// DriveCurrentDirectory.cpp
// Get the current drive directory.

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <iostream>

INT wmain(VOID)
{
    WCHAR CurrentDirectory[MAX_PATH];
    GetFullPathName(L"C:", MAX_PATH, CurrentDirectory, nullptr);

    std::wcout << CurrentDirectory << std::endl;

    // Based on the output of this program, 
    // apparently the current drive directory is set
    // to the current directory of the currently
    // executing process; which is to say, the drive
    // current directory is maintained on a 
    // per-process basis.

    return 0;
}