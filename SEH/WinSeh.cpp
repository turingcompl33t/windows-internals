// WinSeh.cpp
// SEH demo utilizing the native Windows SEH syntax support.

#include <windows.h>
#include <cstdio>

typedef INT NTAPI 
fExceptionFilter(
    EXCEPTION_POINTERS* Pointers
    ); 

fExceptionFilter NopExceptionFilter;
fExceptionFilter FixExceptionFilter;

const int ConstantZero = 0;

int main()
{
    printf("ConstantZero = %d\n", ConstantZero);

    __try 
    {
        const_cast<int&>(ConstantZero) = 1;
    }
    __except(FixExceptionFilter(GetExceptionInformation()))
    {
        // handler code goes here
    }

    printf("ConstantZero = %d\n", ConstantZero);

}

INT NopExceptionFilter(EXCEPTION_POINTERS* Pointers)
{   
    printf("AN EXCEPTION OCCURRED!\n");

    // "I can't handle this"
    return EXCEPTION_CONTINUE_SEARCH;
}

INT FixExceptionFilter(EXCEPTION_POINTERS* Pointers)
{
    printf("AN EXCEPTION OCCURRED!\n");

    if (Pointers->ExceptionRecord->ExceptionCode           != STATUS_ACCESS_VIOLATION || 
        Pointers->ExceptionRecord->ExceptionInformation[0] != EXCEPTION_WRITE_FAULT)
    {
        printf("NOT A WRITE ACCESS VIOLATION\n");

        // not the write access violation we are looking for
        return EXCEPTION_CONTINUE_SEARCH;
    }

    printf("GOT A WRITE ACCESS VIOLATION\n");

    void* WriteAddress = reinterpret_cast<void*>(
        Pointers->ExceptionRecord->ExceptionInformation[1]
        );

    if (!::VirtualProtect(WriteAddress, sizeof(INT), PAGE_READWRITE, nullptr))
    {
        // failed to fix it
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // fixed!
    return EXCEPTION_CONTINUE_EXECUTION;
}