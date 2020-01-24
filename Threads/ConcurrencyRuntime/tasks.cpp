// tasks.cpp
// Demonstration of PPL tasks.

#include <windows.h>
#include <ppl.h>
#include <ppltasks.h>

#include <cstdio>
#include <vector>

using namespace concurrency;

int task_1()
{
	printf("task 1\n");
	
	wait(5000);

	return 1;
}

int wmain()
{
	// default task construction
	auto t = task<int>{};

	// task construction with a free function
	t = task<int>{ task_1 };
	printf("result: %d\n", t.get());

	// task construction with a lambda
	t = task<int>{ [] { printf("task 2\n"); return 2; } };
	printf("result: %d\n", t.get());

	// use of the create_task convenience function
	t = create_task([] 
	{
		printf("task 3\n");
		return 3;
	});
	printf("result: %d\n", t.get());

	// demonstration of PPL continuations
	t = create_task([]
	{
		printf("task 4a\n");
		return 4;
	});
	t = t.then([](int res)
	{
		printf("task 4b\n");
		return res * 10;
	});
	printf("result: %d\n", t.get());

	// demonstration of a logical chaining mechanism
	auto a = create_task([] { printf("task a\n"); });
	auto b = create_task([] { printf("task b\n"); });
	auto c = (a && b).then([] {printf("task c\n"); });
	c.wait();

	return 0;
}