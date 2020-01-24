// WaitObjects.cpp
// Demonstration of Windows thread pool wait objects.

#include <windows.h>

#include <wdl/debug.h>
#include <wdl/event.h>
#include <wdl/exception.h>
#include <wdl/unique_handle.h>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

void __stdcall wait_over(
	PTP_CALLBACK_INSTANCE,
	void*,
	PTP_WAIT,
	TP_WAIT_RESULT
	);

void __stdcall wait_over(
	PTP_CALLBACK_INSTANCE,
	void*,
	PTP_WAIT,
	TP_WAIT_RESULT
	);

int wmain()
{
	// create the wait object
	auto wait = wdl::wait_handle
	{
		::CreateThreadpoolWait(
			wait_over,
			nullptr,
			nullptr
		)
	};

	if (!wait)
	{
		throw wdl::windows_exception{};
	}

	// create a new event object
	auto event = wdl::event{ wdl::event_type::manual_reset };

	// associate the event with the wait object
	::SetThreadpoolWait(wait.get(), event.get(), nullptr);

	// open a registry key of interest
	auto key = open_registry_key(
		HKEY_CURRENT_USER,
		L"Sample",
		KEY_NOTIFY
	);

	// use the registry API to register for a 
	// notification via the event object whenever 
	// the key / value of interest is changed
	// NOTE: latter flag introduced in Windows 8.1

	const auto filter = REG_NOTIFY_CHANGE_LAST_SET |
						REG_NOTIFY_THREAD_AGNOSTIC;

	auto result = ::RegNotifyChangeKeyValue(
		key.get(),
		false,
		filter,
		event.get(),
		true
	);

	if (ERROR_SUCCESS != result)
	{
		throw wdl::windows_exception{};
	}

	// tell the thread pool we want to stop waiting 
	// on the event previously; no new callbacks will be queued
	::SetThreadpoolWait(wait.get(), nullptr, nullptr);

	// now, clear the queue of any pending callbacks that
	// may have been added before we unregistered the wait 
	::WaitForThreadpoolWaitCallbacks(wait.get(), false);

	return STATUS_SUCCESS_I;
}

// callback on wait notification
void __stdcall wait_over(
	PTP_CALLBACK_INSTANCE,
	void*,
	PTP_WAIT,
	TP_WAIT_RESULT
)
{
	TRACE(L"wait is over\n");
}

// registry API helper
wdl::reg_handle open_registry_key(
	HKEY key,
	const wchar_t* path,
	REGSAM access
)
{
	HKEY handle{};

	const auto result = ::RegOpenKeyEx(
		key,
		path,
		0,
		access,
		&handle
	);

	if (ERROR_SUCCESS != result)
	{
		throw wdl::windows_exception{};
	}

	return wdl::reg_handle{ handle };
}