// Cleanup.cpp
// Demonstration of Windows thread pool cleanup groups.

#include <windows.h>

#include <wdl/debug.h>
#include <wdl/thread.h>
#include <wdl/exception.h>
#include <wdl/thread_pool.h>
#include <wdl/unique_handle.h>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr const wchar_t* HARD_WORK = L"hard work";
constexpr const wchar_t* EASY_WORK = L"easy work";

void __stdcall hard_work(
	PTP_CALLBACK_INSTANCE instance,
	void* arg,
	PTP_WORK work
	);
void __stdcall easy_work(PTP_CALLBACK_INSTANCE instance, void* arg);
void __stdcall cleanup(void* object, void* args);

int wmain()
{
	// create a new threadpool cleanup group
	auto cg = wdl::cleanup_group
	{
		::CreateThreadpoolCleanupGroup()
	};

	if (!cg)
	{
		throw wdl::windows_exception{};
	}

	// create a new thread pool environment
	// the environment is what connects the thread pool and the cleanup group
	wdl::tp_environment env{};

	// link the threadpool and the cleanup group
	::SetThreadpoolCallbackCleanupGroup(env.get(), cg.get(), nullptr);

	// create a new threadpool work object
	auto work = ::CreateThreadpoolWork(
		hard_work, 
		reinterpret_cast<void*>(const_cast<wchar_t*>(HARD_WORK)), 
		env.get()
		);

	if (!work)
	{
		throw wdl::windows_exception{};
	}

	// submit the work object to the pool
	::SubmitThreadpoolWork(work);

	// submit a callback to the pool
	if (!::TrySubmitThreadpoolCallback(
		easy_work, 
		reinterpret_cast<void*>(const_cast<wchar_t*>(EASY_WORK)),
		env.get()
	))
	{
		throw wdl::windows_exception{};
	}

	// cleanup all of the objects in the cleanup group
	::CloseThreadpoolCleanupGroupMembers(cg.get(), false, cleanup);

	return STATUS_SUCCESS_I;
}

// callback for CreateThreadPoolWork()
void __stdcall hard_work(
	PTP_CALLBACK_INSTANCE instance, 
	void* arg, 
	PTP_WORK work
	)
{
	auto str = static_cast<wchar_t*>(arg);

	TRACE(L"[%u] %ws\n", wdl::thread_id(), str);
}

// callback for CreateThreadPoolCallback()
void __stdcall easy_work(PTP_CALLBACK_INSTANCE instance, void* arg)
{
	auto str = static_cast<wchar_t*>(arg);

	TRACE("[%u], %ws\n", wdl::thread_id(), str);
}

void __stdcall cleanup(void* object, void* args)
{
	TRACE(L"[%u] cleanup\n", wdl::thread_id());
}