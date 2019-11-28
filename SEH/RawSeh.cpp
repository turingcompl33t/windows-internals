// RawSeh.cpp
// Raw SEH implementation.
//
// To output a PDB file:
//  cl.exe [compiler options] RawSeh.cpp /link /DEBUG
//  (defaults to /DEBUG:FULL)

#include <cstdio>
#include <windows.h>

// ripped from winnt.h
typedef 
EXCEPTION_DISPOSITION NTAPI
fExceptionHandler(
    EXCEPTION_RECORD* ExceptionRecord,
    void*             EstablisherFrame,
    CONTEXT*          ContextRecord,
    void*             DispatcherContext
    );

fExceptionHandler NopExceptionHandler;

const int ConstantZero = 0;

INT main()
{
    NT_TIB* TIB = reinterpret_cast<NT_TIB*>(NtCurrentTeb());

    // create our handler and link it into the list
    EXCEPTION_REGISTRATION_RECORD Registration;
    Registration.Handler = &NopExceptionHandler;
    Registration.Next    = TIB->ExceptionList;
    TIB->ExceptionList   = &Registration;

    printf("ConstantZero = %d\n", ConstantZero);

    const_cast<int&>(ConstantZero) = 1;

    printf("ConstantZero = %d\n", ConstantZero);
    
    // unlink our handler from the list
    TIB->ExceptionList = TIB->ExceptionList->Next;
}

EXCEPTION_DISPOSITION NTAPI NopExceptionHandler(
    EXCEPTION_RECORD* ExceptionRecord,
    void*             EstablisherFrame,
    CONTEXT*          ContextRecord,
    void*             DispatcherContext
)
{
    printf(
        "An exception occurred at address 0x%p, with ExceptionCode 0x%08X\n",
        ExceptionRecord->ExceptionAddress,
        ExceptionRecord->ExceptionCode
    );

    // "I can't handle this"
    return ExceptionContinueSearch;
}