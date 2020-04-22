// DriveCurrentDirectory.cpp
// Get the current drive directory.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 drive_current_directory.cpp

#include <windows.h>
#include <iostream>

int main()
{
    char CurrentDirectory[MAX_PATH];
    GetFullPathNameA("C:", MAX_PATH, CurrentDirectory, nullptr);

    std::cout << CurrentDirectory << std::endl;

    // Based on the output of this program, 
    // apparently the current drive directory is set
    // to the current directory of the currently
    // executing process; which is to say, the drive
    // current directory is maintained on a 
    // per-process basis.

    return 0;
}