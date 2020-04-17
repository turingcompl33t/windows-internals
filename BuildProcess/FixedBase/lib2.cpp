// lib2.cpp
//
// Trying to load two fixed-base DLLs at the same address.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /LD lib2.cpp
//
// Build w/ fixed base
//  cl /EHsc /nologo /std:c++17 /W4 /LD lib2.cpp /link /FIXED

#include <windows.h>
#include <iostream>

BOOL __stdcall DllMain(
    HMODULE       /* instance */,
    unsigned long /* reason */,
    void*         /* reserved */
    )
{
    return TRUE;
}

extern "C" __declspec(dllexport) 
void lib2_func()
{
    std::cout << "Hello from lib2\n";
}