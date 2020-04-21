// server.cpp
//
// Implementation of basic COM library.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /LD server.cpp

#include <windows.h>
#include <iostream>

#include "server.hpp"

struct Instance : IDerived
{
    long m_count;

    Instance() : m_count{0}
    {
        std::cout << "Instance()\n";
    }

    ~Instance()
    {
        std::cout << "~Instance()\n";
    }

    // IUnknown interface implementation

    unsigned long __stdcall AddRef()
    {
        // typically important to ensure that
        // AddRef() is thread safe
        return InterlockedIncrement(&m_count);
    }

    unsigned long __stdcall Release()
    {
        // typically important to ensure that 
        // Release() is thread safe
        long const count = InterlockedDecrement(&m_count);
        
        if (0 == count)
        {
            delete this;
        }

        return count;
    }

    HRESULT __stdcall QueryInterface(
        IID const &id, 
        void **result
        )
    {
        // should validate that result is not nullptr
        // e.g. assertion, exception

        if (id == __uuidof(IBase)
         || id == __uuidof(IDerived)
         || id == __uuidof(IUnknown))
        {
            *result = static_cast<IDerived*>(this);
        }
        else
        {
            *result = nullptr;
            return E_NOINTERFACE;
        }

        // AddRef() and Release() must be called on the
        // same interface (per the rules of COM?)
        static_cast<IUnknown*>(*result)->AddRef();
        
        return S_OK;
    }

    // IBaseOne interface implementation

    void __stdcall BaseMethodOne() override
    {
        std::cout << "BaseMethodOne()\n";
    }

    void __stdcall BaseMethodTwo() override
    {
        std::cout << "BaseMethodTwo()\n";
    }

    // IDervived interface implementation

    void __stdcall DerivedMethod() override
    {
        std::cout << "DerivedMethod()\n";
    }
};

HRESULT __declspec(dllexport) __stdcall 
CreateInstance(IBase** result)
{
    // should validate that result is not nullptr

    // specify that operator new should not throw 
    // on memory allocation failure and instead
    // simply return nullptr
    *result = new (std::nothrow) Instance;

    if (nullptr == *result)
    {
        return E_OUTOFMEMORY;
    }

    (*result)->AddRef();
    return S_OK;
}