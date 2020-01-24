// Timers.cpp
// Demonstration of Windows thread pool timers and timeouts.

#include <windows.h>
#include <thread>
#include <chrono>

#include <wdl/debug.h>
#include <wdl/thread.h>
#include <wdl/exception.h>
#include <wdl/unique_handle.h>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

FILETIME relative_time(unsigned ms);

void __stdcall times_up(
	PTP_CALLBACK_INSTANCE,
	void*,
	PTP_TIMER
	);

int wmain()
{
	using namespace std::chrono_literals;

	// construct a new timer object
	auto timer = wdl::timer_handle
	{
		::CreateThreadpoolTimer(
			times_up,
			nullptr,
			nullptr
		)
	};

	if (!timer)
	{
		throw wdl::windows_exception{};
	}
	
	// multiple methods to specify the timer due time:
	//	- absolute time
	//	- relative time

	auto due_time = relative_time(1000);

	// instruct the thread pool to queue the callback when timer expires
	::SetThreadpoolTimer(timer.get(), &due_time, 0, 0);

	// allow the timer to expire
	std::this_thread::sleep_for(10s);

	return STATUS_SUCCESS_I;
}

// helper to construct FILETIME from relative time interval
FILETIME relative_time(unsigned ms)
{
	union threadpool_time
	{
		INT64 quad;
		FILETIME ft;
	};

	auto t = threadpool_time
	{
		-static_cast<INT64>(ms)
	};

	return t.ft;
}

// timer callback
void __stdcall times_up(
	PTP_CALLBACK_INSTANCE,
	void*,
	PTP_TIMER
)
{
	TRACE(L"timer expired: %u\n", wdl::thread_id());
}