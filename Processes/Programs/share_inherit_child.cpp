// share_inherit_child.cpp
//
// Demonstrates sharing of kernel object handle via inheritance.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 share_inherit_child.cpp

#include <windows.h>
#include <cstdio>

#include <string>

int main(int argc, char* argv[])
{
    HANDLE hEvent;
    ULONG UlongHandle;

    try
    {
        UlongHandle = std::stoul(argv[1]);
    }
    catch(const std::exception& e)
    {
        // lazy error handling;
        // std::stoul may throw invalid_argument
        // or out_of_range exceptions

        printf("[CHILD] Failed to parse arguments\n");
        return 1;
    }

    hEvent = ::ULongToHandle(UlongHandle);

    printf("[CHILD] Got handle to event; waiting for signal\n");

    ::WaitForSingleObject(hEvent, INFINITE);

    printf("[CHILD] Event signaled, exiting\n");
    
    return 0;
}