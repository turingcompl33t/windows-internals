// combinable_perf.cpp
//
// Demonstration of using PPL's combinable to parallelize work.
//
// Build
//	cl /EHsc /nologo /std:c++17 /W4 combinable_perf.cpp

#include <windows.h>
#include <ppl.h>

#include <array>
#include <chrono>
#include <numeric>
#include <algorithm>
#include <iostream>

// naive prime determination function
bool is_prime(int n)
{
    if (n < 2)
    {
        return false;
    }

    for (int i = 2; i < n; ++i)
    {
        if ((n % i) == 0)
        {
            return false;
        }
    }

    return true;
}

template <typename Function>
std::chrono::milliseconds time_call(Function&& fn)
{
    auto start = std::chrono::steady_clock::now();
    fn();
    auto end = std::chrono::steady_clock::now();

    auto duration = end - start;
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
}

int main()
{
    auto c = std::array<int, 20000>{};
    std::iota(std::begin(c), std::end(c), 0); // 0 - 20,000

    auto prime_sum = int{}; 

    auto serial_ms = time_call(
        [&]()
        {
            prime_sum = std::accumulate(std::begin(c), std::end(c), 0, 
            [](int acc, int i)
            {
                return is_prime(i) ? (acc + i) : acc;
            });
        });

    std::cout << "Serial:\n";
    std::cout << "\tResult = " << prime_sum << '\n';
    std::cout << "\tTime   = " << serial_ms.count() << "ms\n";

    prime_sum = 0;
    auto parallel_ms = time_call(
        [&]()
        {
            auto cs = concurrency::critical_section{};
            concurrency::parallel_for_each(std::begin(c), std::end(c), 
            [&](int i)
            {
                cs.lock();
                prime_sum += is_prime(i) ? i : 0;
                cs.unlock();
            });
        });


    std::cout << "Parallel w/out combinable:\n";
    std::cout << "\tResult = " << prime_sum << '\n';
    std::cout << "\tTime   = " << parallel_ms.count() << "ms\n";

    prime_sum = 0;
    auto combinable_ms = time_call(
        [&]()
        {
            auto sum = concurrency::combinable<int>{};
            concurrency::parallel_for_each(std::begin(c), std::end(c),
            [&](int i)
            {
                sum.local() += is_prime(i) ? i : 0;
            });

            prime_sum = sum.combine(std::plus<int>{});
        });

    std::cout << "Parallel w/ combinable:\n";
    std::cout << "\tResult = " << prime_sum << '\n';
    std::cout << "\tTime   = " << combinable_ms.count() << "ms\n";
}

