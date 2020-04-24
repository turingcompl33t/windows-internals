// hello_managed.cpp
//
// Comparing native C++ with C++/CLR
//
// Build
//  cl /nologo /std:c++17 /W4 /CLR hello_managed.cpp

using namespace System;

int main()
{
    Console::WriteLine("Hello World!");
    return 0;
}