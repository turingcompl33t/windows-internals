// client.cpp
//
// Demo application utilizing basic COM library. 
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 client.cpp

// link with the COM library
#pragma comment(lib, "server.lib")

#include <windows.h>

#include "server.hpp"

int main()
{
    IBase*    base;
    IDerived* derived;

    if (S_OK != CreateInstance(&base))
    {
        return 0;
    }

    base->BaseMethodOne();

    // if (S_OK == base->QueryInterface(
    //     __uuidof(IDerived),
    //     reinterpret_cast<void**>(&derived)
    // ))

    // this invocation is equivalent to the more
    // verbose one above thanks to a function template 
    // provided in IUnknown
    if (S_OK == base->QueryInterface(&derived))
    {
        derived->DerivedMethod();
        derived->Release();
    }

    base->Release();

    return 0;
}