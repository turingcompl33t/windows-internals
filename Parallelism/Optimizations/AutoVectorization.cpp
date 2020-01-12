// AutoVectorization.cpp
// Demonstration of vectorization performance impact.
//
// Compile:
//  cl /O2 /EHsc /std:c++17 /Qvec-report:2 AutoVectorization.cpp
//
// Results:
//  Inconclusive: the vectorized and non-vectorized loops complete
//  with nearly identical performance characteristics.

#include <windows.h>

#include "..\..\..\WDL\wdl\performance.h"

constexpr auto size = 10000;

int R1[size];
int R2[size];
int A[size];
int B[size];

void test();

int main()
{
    test();
}

void test()
{
    for (int i = 0; i < size; ++i)
    {
        A[i] = i;
    }

    for (int i = 0; i < size; ++i)
    {
        B[i] = i*2;
    }

    {
        wdl::timer t{"Vectorization Enabled"};

        for (int i = 0; i < size; ++i)
        {
            R1[i] = A[i] + B[i];
        }
    }

    {
        wdl::timer t{"Vectorization Disabled"};

        __pragma(loop(no_vector))
        for (int i = 0; i < size; ++i)
        {
            R2[i] = A[i] + B[i];
        }
    }
}