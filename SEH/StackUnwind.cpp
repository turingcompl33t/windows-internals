// StackUnwind.cpp
// Demonstration of stack unwinding under Windows SEH.

#include <windows.h>
#include <cstdio>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr auto MyErrorCode1 = 0x00112233;
constexpr auto MyErrorCode2 = 0x44556677;

constexpr DWORD ErrorCodeToExceptionCode(DWORD dwErrorCode);
constexpr DWORD ExceptionCodeToErrorCode(DWORD dwExceptionCode);

void a();
void b();
void c();

INT main()
{   
    BOOL abnormal;

    __try
    {
        printf("[+] main() try block\n");

        a();
    }
    __finally
    {
        abnormal = AbnormalTermination();
        printf("[+] main() termination handler; abnormal termination: %s\n",
            abnormal ? "TRUE" : "FALSE");
    }

    return STATUS_SUCCESS_I;
}

void a()
{
    __try
    {
        printf("[+] a() try block\n");

        b();
    }
    __except (GetExceptionCode() == ErrorCodeToExceptionCode(MyErrorCode1) ?
        EXCEPTION_EXECUTE_HANDLER :
        EXCEPTION_CONTINUE_SEARCH
        )
    {
        printf("[+] a() exception handler\n");
    }
}

void b()
{
    __try
    {
        printf("[+] b() try block\n");

        c();
    }
    __finally
    {
        // b() never attempts to handle the exception,
        // but it does always manage to clean up after itself
        printf("[+] b() termination handler\n");
    }
}

void c()
{
    __try
    {
        printf("[+] c() try block\n");

        ::RaiseException(
            ErrorCodeToExceptionCode(MyErrorCode1),  // the user-defined exception code 
            0,                                          // no flags
            0,                                          // 0 arguments
            NULL                                        // NULL arguments array
            );
    }
    __except(GetExceptionCode() == ErrorCodeToExceptionCode(MyErrorCode2) ?
        EXCEPTION_EXECUTE_HANDLER :
        EXCEPTION_CONTINUE_SEARCH
        )
    {
        // c() knows how to handle ExceptionCode2 exceptions
        // unfortunately this won't cut it in this instance...
        // we should never see this printed
        printf("[+] c() exception handler\n");
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