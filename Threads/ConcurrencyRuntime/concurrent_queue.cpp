// concurrent_queue.cpp
// Demonstration of PPL concurrent_queue container.

#include <windows.h>
#include <crtdbg.h>
#include <ppl.h>
#include <concurrent_queue.h>

#include <cstdio>

#define ASSERT _ASSERTE

using namespace concurrency;

void producer(concurrent_queue<int>& c)
{
	for (auto i = 0; i < 10; ++i)
	{
		c.push(i);
	}
}

void consumer(concurrent_queue<int>& c)
{
	for (auto i = 0; i < 10; ++i)
	{
		auto value = int{};
		auto faults = int{};

		while (!c.try_pop(value))
		{
			++faults;

			// compare the number of failed attempts to 
			// consume a value from the queue when the 
			// following line is present vs not present 

			wait(1);
		}
	
		printf("%d (%d faults)\n", value, faults);
	}
}

int wmain()
{
	auto c = concurrent_queue<int>{};

	ASSERT(c.empty());
	ASSERT(c.unsafe_size() == 0);

	parallel_invoke
	(
		[&] { producer(c); },
		[&] { consumer(c); }
	);

	ASSERT(c.empty());
}