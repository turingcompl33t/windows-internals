// libmath.cpp
//
// A simple static library.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /c libmath.cpp
//  lib libmath.obj

#include "libmath.hpp"

namespace libmath
{
    int add(int a, int b)
    {
        return a + b;
    }

    int sub(int a, int b)
    {
        return a - b;
    }

    int mul(int a, int b)
    {
        return a * b;
    }

    int div(int a, int b)
    {
        return a / b;
    }
}

