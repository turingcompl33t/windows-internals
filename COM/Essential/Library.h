// Library.h
// Implementation of basic COM library.

#pragma once

#include <unknwn.h>

struct __declspec(uuid("a17ba869-4430-4d40-8699-6291ee18e2a0")) 
IBase : IUnknown
{
    virtual void __stdcall BaseMethodOne() = 0;
    virtual void __stdcall BaseMethodTwo() = 0;
};

struct __declspec(uuid("c9867764-dc88-456d-b578-9f7452166cd1"))
IDerived : IBase
{
    virtual void __stdcall DerivedMethod() = 0;
};

// factory function
HRESULT __declspec(dllexport) __stdcall CreateInstance(IBase** instance);