/*
 * LibDirectSyscalls.cpp
 * Defines module exports.
 */

#include "pch.h"
#include "LibDirectSyscalls.h"

// initialized to unsupported version
static WinVersion g_WinVersion = WinVersion::Unsupported;

/*
 * LdsInit
 * Initialize the library for future direct system calls
 * - determine the OS version information so we can make appropriate calls.
 */
LIBDIRECTSYSCALLS_API LDS_STATUS LdsInit(void)
{
	RTL_OSVERSIONINFOEXW osInfo;
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);

	_RtlGetVersion RtlGetVersion = (_RtlGetVersion)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetVersion");
	if (!RtlGetVersion)
	{
		return LDS_STATUS_FAIL_INIT;
	}

	// always returns success, per docs
	RtlGetVersion(&osInfo);

	_TCHAR chOSMajorMinor[8];
	_stprintf_s(chOSMajorMinor, _countof(chOSMajorMinor), L"%u.%u", osInfo.dwMajorVersion, osInfo.dwMinorVersion);

	if (0 == _tcsicmp(chOSMajorMinor, L"10.0"))
	{
		// Windows 10 identified
		g_WinVersion = WinVersion::Win10;
	}
	else
	{
		return LDS_STATUS_FAIL_UNSUPPORTED_OS;
	}

	return LDS_STATUS_SUCCESS;
}

LIBDIRECTSYSCALLS_API NTSTATUS LdsOpenProcess(
	PHANDLE ProcessHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PCLIENT_ID ClientId
	)
{
	if (WinVersion::Unsupported == g_WinVersion)
	{
		return LDS_STATUS_FAIL_UNINITIALIZED; 
	}

	NTSTATUS ret;

	switch (g_WinVersion)
	{
	case WinVersion::Win10:
	{
		ret = NtOpenProcess10(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
		break;
	}
	default:
	{
		ret = LDS_STATUS_FAIL_UNSUPPORTED_OS;
		break;
	}
	}

	return ret;
}

LIBDIRECTSYSCALLS_API NTSTATUS LdsProtectVirtualMemory(
	IN HANDLE  ProcessHandle,
	IN PVOID* BaseAddress,
	IN SIZE_T* NumberOfBytesToProtect,
	IN ULONG   NewAccessProtection,
	OUT PULONG OldAccessProtection
	)
{
	if (WinVersion::Unsupported == g_WinVersion)
	{
		return LDS_STATUS_FAIL_UNINITIALIZED;
	}

	NTSTATUS ret;

	switch (g_WinVersion)
	{
	case WinVersion::Win10:
	{
		ret = NtProtectVirtualMemory(ProcessHandle, BaseAddress, NumberOfBytesToProtect, NewAccessProtection, OldAccessProtection);
		break;
	}
	default:
	{
		ret = LDS_STATUS_FAIL_UNSUPPORTED_OS;
		break;
	}
	}

	return ret;
}

LIBDIRECTSYSCALLS_API NTSTATUS LdsWriteVirtualMemory(
	IN HANDLE	hProcess,
	IN PVOID	lpBaseAddress,
	IN PVOID	lpBuffer,
	IN SIZE_T	NumberOfBytesToWrite,
	OUT PSIZE_T NumberOfBytesWritten
	)
{
	if (WinVersion::Unsupported == g_WinVersion)
	{
		return LDS_STATUS_FAIL_UNINITIALIZED;
	}

	NTSTATUS ret;

	switch (g_WinVersion)
	{
	case WinVersion::Win10:
	{
		ret = NtWriteVirtualMemory(hProcess, lpBaseAddress, lpBuffer, NumberOfBytesToWrite, NumberOfBytesWritten);
		break;
	}
	default:
	{
		ret = LDS_STATUS_FAIL_UNSUPPORTED_OS;
		break;
	}
	}

	return ret;
}

LIBDIRECTSYSCALLS_API NTSTATUS LdsQueryInformationProcess(
	IN HANDLE           ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID           ProcessInformation,
	IN ULONG            ProcessInformationLength,
	OUT PULONG          ReturnLength
	)
{
	if (WinVersion::Unsupported == g_WinVersion)
	{
		return LDS_STATUS_FAIL_UNINITIALIZED;
	}

	NTSTATUS ret;

	switch (g_WinVersion)
	{
	case WinVersion::Win10:
	{
		ret = NtQueryInformationProcess10(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
		break;
	}
	default:
	{
		ret = LDS_STATUS_FAIL_UNSUPPORTED_OS;
		break;
	}
	}

	return ret;
}



