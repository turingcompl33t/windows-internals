// HelloCfg.cpp
// App that does nothing other than block on user input
// so one may examine the impact of building with/without
// the Control Flow Guard mitigation enabled.
//
// To observe the impact of CFG on the memory usage of the
// process, build the application with the build script (build.bat)
// which will produce two executables:
//  - CfgDisabled.exe
//  - CfgEnabled.exe
//
// Run both exectuables and check out the differences between the 
// memory usage in Process Explorer; specifically the columns:
//  - Private Bytes -> shows private committed memory usage
//  - Virtual Size -> shows private reserved + committed usage
//
// The Private Bytes stats for the two processes should be relatively
// similar, while the Virtual Size for the process with CFG enabled
// will appear massive - in excess of 2 TB

#include <windows.h>
#include <tchar.h>

#include <iostream>

INT _tmain(INT argc, PTCHAR argv[])
{
    std::cout << "Hello CFG!" << std::endl;
    std::cout << "ENTER to exit" << std::endl;
    std::cin.get();
    return 0;
}