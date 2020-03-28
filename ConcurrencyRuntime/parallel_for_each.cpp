// parallel_for_each.cpp
// Demonstration of PPL parallel_for_each algorithm.

#include <windows.h>
#include <ppl.h>

#include <cstdio>
#include <vector>

using namespace concurrency;

int wmain()
{
	auto v = std::vector<int>{ 1, 2, 3, 4, 5 };

	// the standard for_each provided by STL
	std::for_each(std::begin(v), std::end(v), [](int e)
		{
			printf("%d ", e);
		});

	printf("\n");

	// run the parallel_for_each algorithm twice
	// the output will likely demonstrate that in the first
	// iteration most of the loops are serviced by the same 
	// thread, while in the second they are more widely dispersed
	for (auto i = 0; i < 2; ++i)
	{
		concurrency::parallel_for_each(std::begin(v), std::end(v), [](int e)
		{
			printf("[%u] %d\n", e, ::GetCurrentThreadId());
		});

		printf("\n");
	}

	// NOTE: printf() has internal locking

	return 0;
}