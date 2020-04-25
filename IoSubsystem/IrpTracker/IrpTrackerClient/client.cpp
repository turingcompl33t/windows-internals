// client.cpp
//
// Client application entry point.

#include <windows.h>
#include <winioctl.h>

#include <string>
#include <iostream>
#include <string_view>

constexpr auto const STATUS_SUCCESS_I = 0x0;
constexpr auto const STATUS_FAILURE_I = 0x1;

constexpr auto const SYMLINK_NAME = L"\\??\\IrpTracker";

constexpr auto const IRP_TRACKER_DEVICE = 0x8000;
constexpr auto const IRP_TRACKER_IOCTL_ADD_PENDING
= CTL_CODE(IRP_TRACKER_DEVICE, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS);
constexpr auto const IRP_TRACKER_IOCTL_FLUSH_PENDING
= CTL_CODE(IRP_TRACKER_DEVICE, 0x801, METHOD_NEITHER, FILE_ANY_ACCESS);

void error(
	std::string_view    msg,
	unsigned long const code = ::GetLastError()
)
{
	std::cout << "[-] " << msg
		<< " Code: " << code
		<< std::endl;
}

bool issue_thread_specific_io(HANDLE device)
{
	std::cout << "[+] ENTER to issue thread-specific IO operation\n";
	std::cin.get();

	auto bytes_out = unsigned long{};

	auto ov1 = OVERLAPPED{};
	auto ov2 = OVERLAPPED{};
	ov1.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
	ov2.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

	// issue a thread-specific IO operation
	if (!::DeviceIoControl(
		device,
		static_cast<unsigned long>(
			IRP_TRACKER_IOCTL_ADD_PENDING),
		nullptr, 0u,
		nullptr, 0u,
		&bytes_out,
		&ov1) && ::GetLastError() != ERROR_IO_PENDING)
	{
		error("DeviceIoControl() failed");
	}

	std::cout << "[+] Issued thread-specific IO operation\n"
		<< "[+] ENTER to complete the operation\n";
	std::cin.get();

	if (!::DeviceIoControl(
		device,
		static_cast<unsigned long>(
			IRP_TRACKER_IOCTL_FLUSH_PENDING),
		nullptr, 0u,
		nullptr, 0u,
		&bytes_out,
		&ov2) && ::GetLastError() != ERROR_IO_PENDING)
	{
		error("DeviceIoControl() failed");
	}

	HANDLE events[2];
	events[0] = ov1.hEvent;
	events[1] = ov2.hEvent;

	::WaitForMultipleObjects(2, events, TRUE, INFINITE);

	return true;
}

unsigned long __stdcall worker(void* arg)
{
	auto port = static_cast<HANDLE>(arg);

	auto bytes_xfer = unsigned long{};
	auto completion_key = ULONG_PTR{};
	auto pov = LPOVERLAPPED{};

	// process 2 completions
	for (auto n_completions = 0u; n_completions < 2; ++n_completions)
	{
		if (!::GetQueuedCompletionStatus(port, &bytes_xfer, &completion_key, &pov, INFINITE))
		{
			error("GetQueuedCompletionStatus() failed");
			break;
		}
	}

	return STATUS_SUCCESS_I;
}

bool issue_thread_agnostic_io(HANDLE device)
{
	// create a new IO completion port
	auto port = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	if (NULL == port)
	{
		error("CreateIoCompletionPort() failed");
		return false;
	}

	// associate the device handle with the port
	::CreateIoCompletionPort(device, port, 1, 0);

	// create a new thread to process completions (could also do it on this thread)
	auto worker_thread = ::CreateThread(nullptr, 0, worker, static_cast<void*>(port), 0, nullptr);

	std::cout << "[+] ENTER to issue thread-agnostic IO operation\n";
	std::cin.get();

	auto ov1 = OVERLAPPED{};
	auto ov2 = OVERLAPPED{};
	auto bytes_out = unsigned long{};

	if (!::DeviceIoControl(
		device,
		static_cast<unsigned long>(
			IRP_TRACKER_IOCTL_ADD_PENDING),
		nullptr, 0u,
		nullptr, 0u,
		&bytes_out,
		&ov1) && ::GetLastError() != ERROR_IO_PENDING)
	{
		error("DeviceIoControl() failed");
	}

	std::cout << "[+] Issued thread-agnostic IO operation\n"
		<< "[+] ENTER to complete the operation\n";
	std::cin.get();

	if (!::DeviceIoControl(
		device,
		static_cast<unsigned long>(
			IRP_TRACKER_IOCTL_FLUSH_PENDING),
		nullptr, 0u,
		nullptr, 0u,
		&bytes_out,
		&ov2) && ::GetLastError() != ERROR_IO_PENDING)
	{
		error("DeviceIoControl() failed");
	}

	// wait for worker to process both completions
	::WaitForSingleObject(worker_thread, INFINITE);

	return true;
}

int main()
{
	// acquire a handle to the device
	auto device = ::CreateFileW(
		SYMLINK_NAME,
		GENERIC_READ |
		GENERIC_WRITE,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		nullptr);
	if (INVALID_HANDLE_VALUE == device)
	{
		error("CreateFile() failed");
		return STATUS_FAILURE_I;
	}

	std::cout << "[+] Acquired handle to driver device\n";

	issue_thread_specific_io(device);
	issue_thread_agnostic_io(device);

	std::cout << "[+] Success; exiting\n";

	::CloseHandle(device);

	return STATUS_SUCCESS_I;
}