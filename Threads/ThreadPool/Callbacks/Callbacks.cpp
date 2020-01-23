// Callbacks.cpp
// Demonstration of submitting work to a threadpool via callbacks.

#include <windows.h>
#include <cstdio>
#include <thread>
#include <chrono>

#include <wdl/debug.h>
#include <wdl/thread.h>
#include <wdl/unique_handle.h>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

void __stdcall work(PTP_CALLBACK_INSTANCE, void* arg);

int wmain()
{
	using namespace std::chrono_literals;

	unsigned long id{};

	auto tp = wdl::pool_handle
	{
		::CreateThreadpool(nullptr)
	};

	if (::TrySubmitThreadpoolCallback(
		work,
		&id,
		nullptr
	))
	{
		TRACE(L"submitted work\n");
	}

	// give the callback a chance to run
	std::this_thread::sleep_for(1s);

	TRACE(L"got id: %u\n", id);

	return STATUS_SUCCESS_I;
}

void __stdcall work(PTP_CALLBACK_INSTANCE, void* arg)
{
	auto& id = *static_cast<unsigned long*>(arg);

	id = wdl::thread_id();

	TRACE(L"work %u\n", wdl::thread_id());
}