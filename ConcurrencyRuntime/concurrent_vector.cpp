// concurrent_vector.cpp
//
// Demonstration of PPL concurrent_vector container.
//
// Build
//	cl /EHsc /nologo /std:c++17 /W4 /I C:\Dev\Catch2 concurrent_vector.cpp

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <windows.h>
#include <ppl.h>
#include <concurrent_vector.h>

#include <numeric>

// IMPT: the PPL concurrent_vector does not
// guarantee that the elements of the container 
// are stored contiguously in order, as the 
// std::vector container does, so the concurrent_vector
// cannot be used to produce a predictable binary layout

TEST_CASE("concurrency::concurrent_vector")
{
	auto c = concurrency::concurrent_vector<int>{};

	REQUIRE(c.empty());
	REQUIRE(c.size() == 0);

	// fill the vector in parallel
	concurrency::parallel_for(1, 10001, 
		[&](int i)
		{
			c.push_back(i);
		});
	
	REQUIRE(c.size() == 10000);
	
	auto sum = std::accumulate(std::begin(c), std::end(c), 0);
	REQUIRE(sum == (10000*10001)/2);
}