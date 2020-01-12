// AutoParallelization.cpp
// Demonstration of auto-parallelization support in MSVC.
//
// Compile:
//  cl AutoParallelization.cpp /O2 /Qpar /Qpar-report:1
//
// (Truncated) Output:
//
// --- Analyzing function: void __cdecl test(void)
// AutoParallelization.cpp(24) : info C5011: loop parallelized
//
// --- Analyzing function: main
// AutoParallelization.cpp(24) : info C5011: loop parallelized
//
// Compile:
//  cl AutoParallelization.cpp /O2 /Qpar /Qpar-report:2
//
// (Truncated) Output:
//
// --- Analyzing function: void __cdecl test(void)
// AutoParallelization.cpp(24) : info C5011: loop parallelized
// AutoParallelization.cpp(29) : info C5012: loop not parallelized due to reason '1008'
//
// --- Analyzing function: main
// AutoParallelization.cpp(24) : info C5011: loop parallelized
// AutoParallelization.cpp(29) : info C5012: loop not parallelized due to reason '1008'


#include <windows.h>

int array[1000];

void test();

int main()
{
    test();
}

void test()
{
    __pragma(loop(hint_parallel(6)))
    for (int i = 0; i < 1000; ++i)
    {
        array[i] = array[i] + 1;
    }

    for (int i = 1000; i < 2000; ++i)
    {
        array[i] = array[i] + 1;
    }
}