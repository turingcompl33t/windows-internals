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

constexpr auto const SYMLINK_NAME = L"\\??\\TrackingIrps";

constexpr auto const IRP_TRACKER_DEVICE = 0x8000;
constexpr auto const IRP_TRACKER_IOCTL_ADD_PENDING
	= CTL_CODE(IRP_TRACKER_DEVICE, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS);
constexpr auto const IRP_TRACKER_IOCTL_FLUSH_PENDING
	= CTL_CODE(IRP_TRACKER_DEVICE, 0x801, METHOD_NEITHER, FILE_ANY_ACCESS);

void error(
	std::string_view msg,
	unsigned long const code = ::GetLastError()
	)
{
	std::cout << "[-] " << msg
		<< " Code: " << code
		<< std::endl;
}

bool enable_privilege(HANDLE token, wchar_t const* name)
{
	auto luid       = LUID{};
	auto privileges = TOKEN_PRIVILEGES{};

	if (!LookupPrivilegeValueW(nullptr, name, &luid))
	{
		error("LookupPrivilegeValue() failed");
		::CloseHandle(token);
		return false;
	}

	privileges.PrivilegeCount = 1;
	privileges.Privileges[0].Luid = luid;
	privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!::AdjustTokenPrivileges(
		token, 
		FALSE, 
		&privileges, 
		sizeof(TOKEN_PRIVILEGES), 
		nullptr, nullptr))
	{
		error("AdjustTokenPrivileges() failed");
		::CloseHandle(token);
		return false;
	}

	auto const success = ::GetLastError() != ERROR_NOT_ALL_ASSIGNED;

	::CloseHandle(token);

	return success;
}

int main()
{	
	auto this_process_token = HANDLE{};
	::OpenProcessToken(::GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY, &this_process_token);

	auto token = HANDLE{};
	if (!::DuplicateToken(this_process_token, SecurityImpersonation, &token))
	{
		error("DuplicateToken() failed");
		return STATUS_FAILURE_I;
	}

	if (!enable_privilege(token, L"SeLockMemoryPrivilege"))
	{
		std::cout << "[-] Failed to enable SeLockMemory\n";
		return STATUS_FAILURE_I;
	}

	std::cout << "[+] Successfully enabled SeLockMemory\n";

	auto device = ::CreateFileW(
		SYMLINK_NAME,
		GENERIC_READ  | 
		GENERIC_WRITE |
		FILE_READ_ATTRIBUTES,
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

	std::cout << "[+] Acquired handle to driver device\n"
		<< "[+] ENTER to issue thread-specific IO operation\n";
	std::cin.get();

	auto bytes_out = unsigned long{};

	// issue a thread-specific IO operation
	auto ov1 = OVERLAPPED{};
	ov1.hEvent = ::CreateEventW(nullptr, FALSE, TRUE, nullptr);

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
		<< "[+] ENTER to issue thread-agnostic IO operation\n";
	std::cin.get();

	// issue a thread-agnostic IO operation
	auto ov2 = OVERLAPPED{};
	ov2.hEvent = ::CreateEventW(nullptr, FALSE, TRUE, nullptr);

	if (!::SetFileIoOverlappedRange(
		device,
		reinterpret_cast<unsigned char*>(&ov2),
		sizeof(OVERLAPPED)))
	{
		error("SetFileIoOverlappedRange() failed");
	}

	if (!::DeviceIoControl(
		device,
		static_cast<unsigned long>(
			IRP_TRACKER_IOCTL_ADD_PENDING),
		nullptr, 0u,
		nullptr, 0u,
		&bytes_out,
		&ov2) && ::GetLastError() != ERROR_IO_PENDING)
	{
		error("DeviceIoControl() failed");
	}

	std::cout << "[+] Issued thread-agnostic IO operation\n"
		<< "[+] ENTER to cleanup and exit\n";

	// wait
	std::cin.get();

	// flush the pending requests
	::DeviceIoControl(
		device,
		static_cast<unsigned long>(
			IRP_TRACKER_IOCTL_FLUSH_PENDING),
		nullptr, 0,
		nullptr, 0,
		&bytes_out,
		nullptr);

	::CloseHandle(device);

	return STATUS_SUCCESS_I;
}