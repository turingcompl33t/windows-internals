// PerformanceTest.cpp
// Comparison of performance between various synchronization primitives.

#include <windows.h>
#include <process.h>
#include <cstdio>
#include <string>

#include <wdl/performance.h>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr auto N_ITERS = 1000;

struct CriticalSectionTestState
{
	CRITICAL_SECTION cr;
	long long        count;
};

struct MutexTestState
{
	HANDLE    mutex;
	long long count;
};

struct InterlockedTestState
{
	volatile long long count;
};

void run_critical_section_test(unsigned long n_threads);
unsigned int __stdcall critical_section_fn(void* args);

void run_mutex_test(unsigned long n_threads);
unsigned int __stdcall mutex_fn(void* args);

void run_interlocked_test(unsigned long n_threads);
unsigned int __stdcall interlocked_fn(void* args);

void log_info(const std::string& msg);
void log_warning(const std::string& msg);
void log_error(
	const std::string& msg,
	const unsigned long error = ::GetLastError()
	);

int wmain(int argc, wchar_t* argv[])
{
	if (argc != 2)
	{
		log_warning("Error: invalid arguments");
		log_warning("Usage: PerformanceTest.exe <THREAD COUNT>");
		return STATUS_FAILURE_I;
	}

	unsigned n_threads;

	try
	{
		n_threads = std::stoul(argv[1]);
	}
	catch (const std::exception& e)
	{
		auto msg = std::string{ "Failed to parse arguments; " } + e.what();
		log_warning(msg);
		
		return STATUS_FAILURE_I;
	}

	if (n_threads > MAXIMUM_WAIT_OBJECTS)
	{
		log_warning("Thread count may not exceed MAXIMUM_WAIT_OBJECTS; truncating");
		n_threads = MAXIMUM_WAIT_OBJECTS;
	}

	run_critical_section_test(n_threads);
	run_mutex_test(n_threads);
	run_interlocked_test(n_threads);

	return STATUS_SUCCESS_I;
}

void run_critical_section_test(unsigned long n_threads)
{
	CriticalSectionTestState cr_state{};
	::InitializeCriticalSection(&cr_state.cr);

	HANDLE handles[MAXIMUM_WAIT_OBJECTS];

	{
		wdl::timer t{ "Critical Section Test" };

		for (unsigned i = 0; i < n_threads; ++i)
		{
			handles[i] = (HANDLE)_beginthreadex(
				nullptr,
				0,
				critical_section_fn,
				&cr_state,
				0,
				0
			);
		}

		::WaitForMultipleObjects(n_threads, handles, true, INFINITE);
	}
}

unsigned int __stdcall critical_section_fn(void* args)
{
	auto cr_args = static_cast<CriticalSectionTestState*>(args);

	for (auto i = 0; i < N_ITERS; ++i)
	{
		::EnterCriticalSection(&cr_args->cr);
		cr_args->count++;
		::LeaveCriticalSection(&cr_args->cr);
	}

	return STATUS_SUCCESS_I;
}

void run_mutex_test(unsigned long n_threads)
{
	MutexTestState mutex_state{};
	mutex_state.mutex = ::CreateMutexW(
		nullptr,
		false,
		nullptr
	);

	if (NULL == mutex_state.mutex)
	{
		log_error("Failed to create mutex");
		return;
	}

	HANDLE handles[MAXIMUM_WAIT_OBJECTS];

	{
		wdl::timer t{ "Mutex Test" };

		for (unsigned i = 0; i < n_threads; ++i)
		{
			handles[i] = (HANDLE)_beginthreadex(
				nullptr,
				0,
				mutex_fn,
				&mutex_state,
				0,
				0
			);
		}

		::WaitForMultipleObjects(n_threads, handles, true, INFINITE);
	}

	::CloseHandle(mutex_state.mutex);
}

unsigned int __stdcall mutex_fn(void* args)
{
	auto mutex_args = static_cast<MutexTestState*>(args);

	for (auto i = 0; i < N_ITERS; ++i)
	{
		::WaitForSingleObject(mutex_args->mutex, INFINITE);
		mutex_args->count++;
		::ReleaseMutex(mutex_args->mutex);
	}

	return STATUS_SUCCESS_I;
}

void run_interlocked_test(unsigned long n_threads)
{
	InterlockedTestState interlocked_state{};

	HANDLE handles[MAXIMUM_WAIT_OBJECTS];

	{
		wdl::timer t{ "Interlocked Test" };

		for (unsigned i = 0; i < n_threads; ++i)
		{
			handles[i] = (HANDLE)_beginthreadex(
				nullptr,
				0,
				interlocked_fn,
				&interlocked_state,
				0,
				0
			);
		}
	}
}

unsigned int __stdcall interlocked_fn(void* args)
{
	auto interlocked_args = static_cast<InterlockedTestState*>(args);

	for (auto i = 0; i < N_ITERS; ++i)
	{
		_InterlockedIncrement64(&interlocked_args->count);
	}

	return STATUS_SUCCESS_I;
}

void log_info(const std::string& msg)
{
	printf("[+] %s\n", msg.c_str());
}

void log_warning(const std::string& msg)
{
	printf("[-] %s\n", msg.c_str());
}

void log_error(
	const std::string& msg, 
	const unsigned long error
	)
{
	printf("[!] %s\n", msg.c_str());
	printf("[!] GLE: %u\n", error);
}
