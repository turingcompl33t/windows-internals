// parallel_transform.cpp
// Demonstration of PPL parallel_transform algorithm.

#include <windows.h>
#include <ppl.h>

#include <cstdio>
#include <vector>

using namespace concurrency;

int wmain()
{
	auto v = std::vector<int>{ 1, 2, 3, 4, 5 };

	// just a parallel version of std::transform
	concurrency::parallel_transform(std::begin(v), std::end(v), std::begin(v), [](int e)
	{
		return e * 2;
	});

	for (const auto& e : v)
	{
		printf("%d ", e);
	}

	printf("\n");

	return 0;
}