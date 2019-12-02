// ContainingRecord.cpp
// Re-implementation of CONTAINING_RECORD macro.

#pragma warning(disable : 4005)  // macro redefinition

#define UNICODE
#define _UNICODE

#include <windows.h>

#include <memory>
#include <iostream>

#define CONTAINING_RECORD(addr, type, member) ((type*)((PCHAR)(addr) - (ULONG_PTR)(&((type*)nullptr)->member)))

struct TestType
{   
    TestType(DWORD a0, DWORD a1, DWORD a2, DWORD a3) 
        : dw0{a0}, dw1{a1}, dw2{a2}, dw3{a3} {};
    ~TestType() = default;

    DWORD dw0;
    DWORD dw1;
    DWORD dw2;
    DWORD dw3;
};

INT wmain()
{
    // yea yea, no raw "new" and "delete" and all that
    TestType* pTest0 = new TestType{0, 1, 2, 3};

    std::cout << "USING ORIGINAL POINTER" << '\n';
    std::cout << "DW0 = " << pTest0->dw0 << '\n';
    std::cout << "DW1 = " << pTest0->dw1 << '\n';
    std::cout << "DW2 = " << pTest0->dw2 << '\n';
    std::cout << "DW3 = " << pTest0->dw3 << '\n';

    // get a pointer to the final member of the type
    PDWORD pDw3 = &(pTest0->dw3);

    // now use it to get back to the containing record
    TestType* pTest1 = CONTAINING_RECORD(pDw3, TestType, dw3);

    // did it work?
    std::cout << "USING DERIVED POINTER" << '\n';
    std::cout << "DW0 = " << pTest1->dw0 << '\n';
    std::cout << "DW1 = " << pTest1->dw1 << '\n';
    std::cout << "DW2 = " << pTest1->dw2 << '\n';
    std::cout << "DW3 = " << pTest1->dw3 << '\n';

    delete pTest0;

    return 0;
}