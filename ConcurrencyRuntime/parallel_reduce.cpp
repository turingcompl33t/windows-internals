// parallel_invoke_reduce.cpp
//
// Demonstration of PPL parallel_invoke and parallel_reduce algorithms.
//
// Build
//	cl /EHsc /nologo /std:c++17 /W4 /I C:\Dev\Catch2 parallel_invoke_reduce.cpp

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <windows.h>
#include <ppl.h>

#include <vector>
#include <numeric>

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

TEST_CASE("concurrency::parallel_invoke() and concurrency::parallel_reduce() \
	can be composed arbitrarily")
{
	auto c = std::vector<int>{};
	std::generate_n(
		std::back_inserter(c), 
		10000, 
		[n = 1]()mutable{ return n++; });

	auto s     = concurrency::parallel_reduce(std::begin(c), std::end(c), 0);
	auto s_exp = std::accumulate(std::begin(c), std::end(c), 0);

	REQUIRE(s == s_exp);
}

TEST_CASE("")
{
	auto c = std::vector<int>{};
	std::generate_n(
		std::back_inserter(c), 
		10000, 
		[n = 1]()mutable{ return n++; });

	concurrency::parallel_transform(
		std::begin(c), 
		std::end(c), 
		std::begin(c), 
		[](int i)
		{
			return is_prime(i) ? i : 0;
		});

	auto p = concurrency::parallel_reduce(std::begin(c), std::end(c), 0);

	// reset container
	std::iota(std::begin(c), std::end(c), 1);

	auto p_exp = std::accumulate(std::begin(c), std::end(c), 0,
		[](int acc, int v){ return is_prime(v) ? acc + v : acc; });

	REQUIRE(p == p_exp);
}