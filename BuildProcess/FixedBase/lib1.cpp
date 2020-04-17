// lib1.cpp
//
// Trying to load two fixed-base DLLs at the same address.
//
// Build w/out fixed base
//  cl /EHsc /nologo /std:c++17 /W4 /LD lib1.cpp
//
// Build w/ fixed base
//  cl /EHsc /nologo /std:c++17 /W4 /LD lib1.cpp /link /FIXED

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
void lib1_func()
{
    std::cout << "Hello from lib1\n";
}