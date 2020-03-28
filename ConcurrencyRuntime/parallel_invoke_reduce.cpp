// parallel_invoke_reduce.cpp
// Demonstration of PPL parallel_invoke and parallel_reduce algorithms.

#include <windows.h>
#include <ppl.h>

#include <cstdio>
#include <vector>

using namespace concurrency;

template <typename C>
typename C::value_type sum(const C& c)
{
	// parallel version of std::accumulate()
	return concurrency::parallel_reduce(std::begin(c), std::end(c), 0);
}

template <typename C>
typename C::value_type product(const C& c)
{
	// parallel version of std::accumulate()
	return concurrency::parallel_reduce(std::begin(c), std::end(c), 1, [](int left, int right)
	{
		return left * right;
	});
}

int wmain()
{
	auto v = std::vector<int>{ 1, 2, 3, 4, 5 };

	auto s = int{};
	auto p = int{};

	concurrency::parallel_invoke
	(
		[&] {s = sum(v); },
		[&] {p = product(v); }
	);

	printf("sum: %d\n", s);
	printf("product: %d\n", p);

	return 0;
}