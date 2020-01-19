// CombinedBlocks.cpp
// Demonstration of combining exception handling and termination handler blocks.

#include <windows.h>
#include <cstdio>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr auto MyErrorCode1 = 0x00112233;
constexpr auto MyErrorCode2 = 0x44556677;

VOID DoNestedSeh();

constexpr DWORD ErrorCodeToExceptionCode(DWORD dwErrorCode);
constexpr DWORD ExceptionCodeToErrorCode(DWORD dwExceptionCode);

INT main()
{
    __try
    {
        printf("[+] main() try block\n");

        DoNestedSeh();
    }
    __except(GetExceptionCode() == ErrorCodeToExceptionCode(MyErrorCode1) ?
        EXCEPTION_EXECUTE_HANDLER :
        EXCEPTION_CONTINUE_SEARCH
    )
    {
        printf("[+] main() exception handler\n");
    }

    return STATUS_SUCCESS_I;
}

VOID DoNestedSeh()
{
    BOOL abnormal;

    __try
    {
        printf("[+] DoNestedSeh() outer try block\n");

        __try
        {
            printf("[+] DoNestedSeh() inner try block\n");

            ::RaiseException(
                ErrorCodeToExceptionCode(MyErrorCode2),
                0,
                0,
                NULL
                );
        }
        __except(GetExceptionCode() == ErrorCodeToExceptionCode(MyErrorCode2) ?
            EXCEPTION_EXECUTE_HANDLER :
            EXCEPTION_CONTINUE_SEARCH
        )
        {
            // this inner exception handler knows how to handle MyErrorCode2
            printf("[+] DoNestedSeh() inner exception handler\n");

            // suppose that during processing of this exception, another is thrown...
            ::RaiseException(
                ErrorCodeToExceptionCode(MyErrorCode1),
                0,
                0,
                NULL
                );
        }
    }
    __finally
    {
        abnormal = AbnormalTermination();

        printf("[+] DoNestedSeh() outer termination handler; abnormal termination: %s\n",
            abnormal ? "TRUE" : "FALSE");
    }
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