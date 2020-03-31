// critical_section.cpp
//
// Demonstration of PPL cooperative synchronization with critical sections.
//
// Build
//	cl /EHsc /nologo /std:c++17 /W4 /I C:\Dev\Catch2 critical_section.cpp

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <windows.h>
#include <ppl.h>

#include <thread>
#include <vector>
#include <algorithm>

// IMPT: critical_section is merely one of the
// synchronization types defined by the concurrency
// runtime; the important distinction between these
// primitives and the primitives provided by the OS
// is that these primitives are cooperative and are
// only meant to be used on top of the runtime; 
// trying to utilize the critical_section shown
// below to synchronize standard windows threads
// created with CreateThread() would result in very
// poor performance

constexpr auto N_THREADS             = 10;
constexpr auto OPERATIONS_PER_THREAD = 10000;

void worker(
	concurrency::critical_section& cs, 
	unsigned long long&            count
	)
{
	for (auto i = 0u; i < OPERATIONS_PER_THREAD; ++i)
	{
		cs.lock();
		++count;
		cs.unlock();
	}
}

TEST_CASE("concurrency::critical_section protects shared data")
{
	auto cs    = concurrency::critical_section{};
	auto count = unsigned long long{};

	auto threads = std::vector<std::thread>{};
	std::generate_n(
		std::back_inserter(threads), 
		N_THREADS, 
		[&]()
		{
			return std::thread{worker, std::ref(cs), std::ref(count)};
		});
	
	std::for_each(std::begin(threads), std::end(threads), [](auto& t){ t.join(); });

	REQUIRE(count == N_THREADS*OPERATIONS_PER_THREAD);
}

void worker_v2(
	concurrency::critical_section& cs, 
	unsigned long long&            count
	)
{
	for (auto i = 0u; i < OPERATIONS_PER_THREAD; ++i)
	{
		auto guard = concurrency::critical_section::scoped_lock{cs};
		++count;
	}
}

TEST_CASE("concurrency::critical section can be used with a scoped lock")
{
	auto cs    = concurrency::critical_section{};
	auto count = unsigned long long{};

	auto threads = std::vector<std::thread>{};
	std::generate_n(
		std::back_inserter(threads), 
		N_THREADS, 
		[&]()
		{
			return std::thread{worker_v2, std::ref(cs), std::ref(count)};
		});
	
	std::for_each(std::begin(threads), std::end(threads), [](auto& t){ t.join(); });

	REQUIRE(count == N_THREADS*OPERATIONS_PER_THREAD);
}