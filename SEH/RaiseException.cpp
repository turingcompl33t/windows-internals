// RaiseException.cpp
// Demo of raising a user-defined exception.

#include <windows.h>
#include <cstdio>

// exception code format:
//  bits  27 - 0: error code
//  bit       28: reserved, cleared by Windows
//  bit       29: MS / "customer" (not MS)
//  bits 31 - 30: severity

// user exception code -> formatted exception code
#define ENCODE_EXCEPTION_CODE(x) ((0x0FFFFFFF & x) | (0xE0000000))
// formatted exception code -> user exception code
#define DECODE_EXCEPTION_CODE(x) (0x0FFFFFFF & x)

constexpr auto MyErrorCode = 0x00112233;

typedef INT NTAPI 
fExceptionFilter(
    EXCEPTION_POINTERS* Pointers
    );

fExceptionFilter MyExceptionFilter;
constexpr DWORD ErrorCodeToExceptionCode(DWORD dwErrorCode);
constexpr DWORD ExceptionCodeToErrorCode(DWORD dwExceptionCode);

INT main()
{
    __try
    {
        RaiseException(
            ErrorCodeToExceptionCode(MyErrorCode),  // the user-defined exception code 
            0,                                      // no flags
            0,                                      // 0 arguments
            NULL                                    // NULL arguments array
            );
    }
    __except(MyExceptionFilter(GetExceptionInformation()))
    {
        printf("HANDLING USER DEFINED EXCEPTION\n");
    }

    printf("EXITING PROGRAM NORMALLY\n");

    return 0;
}

INT MyExceptionFilter(EXCEPTION_POINTERS* Pointers)
{
    auto dwErrorCode = ExceptionCodeToErrorCode(Pointers->ExceptionRecord->ExceptionCode);

    printf("GOT AN EXCEPTION, CODE = 0x%08X\n", dwErrorCode);

    if (dwErrorCode == MyErrorCode)
    {
        printf("EXCEPTION IS USER DEFINED, EXECUTING HANDLER\n");
        return EXCEPTION_EXECUTE_HANDLER;
    }

    printf("EXCEPTION IS NOT USER DEFINED, CONTINUING SEARCH\n");

    return EXCEPTION_CONTINUE_SEARCH;
}

constexpr DWORD ErrorCodeToExceptionCode(DWORD dwErrorCode)
{
    // hardcoded with Error severity, customer exception
    return ((0x0FFFFFFF & dwErrorCode) | 0xE0000000);
}

constexpr DWORD ExceptionCodeToErrorCode(DWORD dwExceptionCode)
{
    return (0x0FFFFFFF & dwExceptionCode);
}