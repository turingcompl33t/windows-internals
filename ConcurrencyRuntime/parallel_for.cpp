// parallel_for.cpp
// Demonstration of PPL parallel_for algorithm.

#include <windows.h>
#include <ppl.h>

#include <cstdio>
#include <vector>

using namespace concurrency;

int wmain()
{
	for (int i = 0; i < 10; ++i)
	{
		printf("%d ", i);
	}

	printf("\n");

	// rewritten version of the above, except executed in parallel 
	concurrency::parallel_for(0, 10, [](int i)
	{
		printf("%d ", i);
	});
}