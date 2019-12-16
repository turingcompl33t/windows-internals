// InheritParent.cpp
// Demonstration of sharing handles via inheritance.

#define UNICODE
#define _UNICODE

#include <windows.h>

#include <cstdio>
#include <string>

INT main(INT argc, PCHAR argv[])
{
    HANDLE hEvent;
    ULONG UlongHandle;

    try
    {
        UlongHandle = std::stoul(argv[1]);
    }
    catch(const std::exception& e)
    {
        printf("[CHILD] Failed to parse arguments\n");
        return 1;
    }

    hEvent = ULongToHandle(UlongHandle);

    printf("[CHILD] Got handle to event; waiting for signal\n");

    WaitForSingleObject(hEvent, INFINITE);

    printf("[CHILD] Event signaled, exiting\n");
    
    return 0;
}