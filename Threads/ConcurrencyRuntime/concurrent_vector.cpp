// concurrent_vector.cpp
// Demonstration of PPL concurrent_vector container.

#include <windows.h>
#include <crtdbg.h>
#include <ppl.h>
#include <concurrent_vector.h>

#include <cstdio>

#define ASSERT _ASSERTE

using namespace concurrency;

// IMPT: the PPL concurrent_vector does not
// guarantee that the elements of the container 
// are stored contiguously in order, as the 
// std::vector container does, so the concurrent_vector
// cannot be used to produce a predictable binary layout

int wmain()
{
	auto c = concurrent_vector<int>{};

	ASSERT(c.empty());
	ASSERT(c.size() == 0);

	auto l = { 1, 2, 3, 4, 5 };
	c = concurrent_vector<int>{ std::begin(l), std::end(l) };

	ASSERT(!c.empty());
	ASSERT(c.size() == 5);

	// NOTE: not a thread safe operation
	c.clear();

	// push_back, however, is thread safe
	// so we can invoke it in parallel_for like so
	parallel_for(0, 10, [&](int i)
	{
		c.push_back(i);
	});

	for (const auto e : c)
	{
		printf("%d ", e);
	}

	printf("\n");

	return 0;
}