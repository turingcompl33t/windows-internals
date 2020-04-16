// client.cpp
//
// libmath consumer.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 client.cpp /link libmath.lib

#include <iostream>

#include "libmath.hpp"

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

int main()
{
    auto const a = 10;
    auto const b = 5;

    std::cout << a << " + " << b
        << " = " << libmath::add(a, b)
        << '\n';
    
    std::cout << a << " - " << b
        << " = " << libmath::sub(a, b)
        << '\n';

    std::cout << a << " * " << b
        << " = " << libmath::mul(a, b)
        << '\n';

    std::cout << a << " / " << b
        << " = " << libmath::div(a, b)
        << '\n';

    return STATUS_SUCCESS_I;
}