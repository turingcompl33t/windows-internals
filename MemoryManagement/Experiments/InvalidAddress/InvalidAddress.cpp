// InvalidAddress.cpp
// Simple program to trigger an access violation on address 0x0.

#include <windows.h>
#include <cstdio>

int main()
{
    __debugbreak();

    int* addr = 0;

    __try
    {
        (*addr) = 1;
    }
    __except(GetExceptionCode() == STATUS_ACCESS_VIOLATION ? 
        EXCEPTION_EXECUTE_HANDLER : 
        EXCEPTION_CONTINUE_SEARCH
        )
    {
        puts("Access Violation");
    }

    puts("Hey, we got here!");

    return 0;
}