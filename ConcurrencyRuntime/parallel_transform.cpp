// parallel_transform.cpp
//
// Demonstration of PPL parallel_transform algorithm.
//
// Build
//	cl /EHsc /nologo /std:c++17 /W4 /I C:\Dev\Catch2 parallel_transform.cpp

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <windows.h>
#include <ppl.h>

#include <vector>
#include <numeric>

using namespace concurrency;

TEST_CASE("")
{
	auto c = std::vector<int>{};
	std::generate_n(
		std::back_inserter(c),
		10000,
		[n = 1]() mutable
		{
			return n++;
		});

	concurrency::parallel_transform(
		std::begin(c),
		std::end(c),
		std::begin(c),
		[](int i)
		{
			return i*2;
		});

	// parallel sum
	auto p = std::accumulate(std::begin(c), std::end(c), 0);

	// reset the vector 
	std::iota(std::begin(c), std::end(c), 1);

	// serial sum
	auto s = std::transform_reduce(
		std::begin(c), 
		std::end(c), 
		0,
		std::plus{},
		[](int i){ return i*2; });

	REQUIRE(s == p); 
}