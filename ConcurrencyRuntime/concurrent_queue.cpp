// concurrent_queue.cpp
//
// Demonstration of PPL concurrent_queue container.
//
// Build
//	cl /EHsc /nologo /std:c++17 /W4 /I C:\Dev\Catch2 concurrent_queue.cpp

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <windows.h>
#include <ppl.h>
#include <concurrent_queue.h>

constexpr auto N_OPERATIONS = 10000;

void producer(concurrency::concurrent_queue<int>& c)
{
	for (auto i = 0u; i < N_OPERATIONS; ++i)
	{
		c.push(i);
	}
}

void consumer(concurrency::concurrent_queue<int>& c)
{
	auto value = int{};

	for (auto i = 0u; i < N_OPERATIONS; ++i)
	{
		while (!c.try_pop(value))
		{
			// compare the number of failed attempts to 
			// consume a value from the queue when the 
			// following line is present vs not present 

			concurrency::wait(1);
		}
	}
}

TEST_CASE("concurrency::concurrent_queue()")
{
	auto c = concurrency::concurrent_queue<int>{};

	REQUIRE(c.empty());
	REQUIRE(c.unsafe_size() == 0);

	concurrency::parallel_invoke(
		[&]{ producer(c); },
		[&]{ consumer(c); }
	);

	// production and consumption must be equivalent
	REQUIRE(c.empty());
}