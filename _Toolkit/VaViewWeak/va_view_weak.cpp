// VaViewWeak.cpp
// Weak version of VaView that runs without the aid of KMD.
//
// Specifically, in this version we do not request PROCESS_ALL_ACCESS
// to the address space of the specified process, and instead we
// request merely PROCESS_QUERY_INFORMATION. Even this weakened
// version will fail for many processes of interest, such as protected
// processes and PPLs (i.e. all system processes...) because the only
// access mask allowed is PROCESS_QUERY_LIMITED_INFORMATION.

#include <windows.h>

#define UNICODE
#define _UNICODE

#pragma warning(disable : 4703)   // uninitialized variables

#include <windows.h>
#include <Psapi.h>

#include <cstdio>
#include <string>
#include <stdexcept>

#include <WtkCommon.h>

VOID ShowMemoryMap(HANDLE hProcess);
const CHAR* ProtectionToString(DWORD protect);
const CHAR* StateToString(DWORD state);
const CHAR* MemoryTypeToString(DWORD type);
const CHAR* GetExtraData(HANDLE hProcess, MEMORY_BASIC_INFORMATION& mbi);

INT main(INT argc, PCHAR argv[])
{
	if (argc != 2)
	{
		printf("[-] Invalid arguments\n");
		printf("[-] Usage: %s <PID>\n", argv[0]);
		return WTK_STATUS_INVALID_ARGUMENTS;
	}

	ULONG pid;
	HANDLE hProcess;

	auto status = WTK_STATUS_SUCCESS;
	auto rollback = 0;

	try
	{
		pid = std::stoul(argv[1]);
	}
	catch (std::invalid_argument&)
	{
		printf("[-] Failed to parse arguments (invalid_argument)\n");
		status = WTK_STATUS_MALFORMED_ARGUMENTS;
		goto CLEANUP;
	}
	catch (std::out_of_range&)
	{
		printf("[-] Failed to parse arguments (out_of_range)\n");
		status = WTK_STATUS_MALFORMED_ARGUMENTS;
		goto CLEANUP;
	}

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (NULL == hProcess)
	{
		printf("[-] Failed to acquire handle to process; GLE: %u\n", GetLastError());
		status = WTK_STATUS_OPERATION_FAILED;
		goto CLEANUP;
	}

	// acquired handle to process
	rollback = 1;

	printf("[+] Successfully acquired handle to process %u\n", pid);

	ShowMemoryMap(hProcess);

CLEANUP:
	switch (rollback)
	{
	case 1:
		CloseHandle(hProcess);
	case 0:
		break;
	}

	return status;
}

VOID ShowMemoryMap(HANDLE hProcess)
{
	ULONGLONG address = 0;
	printf("  %-16s %-16s %15s %16s %26s %10s\n", "Address", "Size (bytes)", "Protection", "State", "Allocation Protection", "Type");
	printf("-----------------------------------------------------------------------------------------------------------\n");

	do {
		MEMORY_BASIC_INFORMATION mbi = { 0 };
		auto size = ::VirtualQueryEx(hProcess, reinterpret_cast<PVOID>(address), &mbi, sizeof(mbi));
		if (size == 0)
		{
			break;
		}

		printf("%16p %16llX %-27s %-10s %-27s %-8s %s\n",
			mbi.BaseAddress,
			mbi.RegionSize,
			mbi.State == MEM_COMMIT ? ProtectionToString(mbi.Protect) : "",
			StateToString(mbi.State),
			mbi.State == MEM_FREE ? "" : ProtectionToString(mbi.AllocationProtect),
			mbi.State == MEM_FREE ? "" : MemoryTypeToString(mbi.Type), GetExtraData(hProcess, mbi));
		address += mbi.RegionSize;
	} while (TRUE);

	printf("-----------------------------------------------------------------------------------------------------------\n");
}

const CHAR* ProtectionToString(DWORD protect)
{
	static CHAR text[256];
	BOOL guard = (protect & PAGE_GUARD) == PAGE_GUARD;
	protect &= ~PAGE_GUARD;

	const CHAR* protection = "Unknown";
	switch (protect) {
	case PAGE_EXECUTE: protection = "Execute"; break;
	case PAGE_EXECUTE_READ: protection = "Execute/Read";
	case PAGE_READONLY: protection = "Read Only";
	case PAGE_NOACCESS: protection = "No Access";
	case PAGE_READWRITE: protection = "Read/Write";
	case PAGE_WRITECOPY: protection = "Write Copy";
	case PAGE_EXECUTE_READWRITE: protection = "Execute/Read/Write";
	case PAGE_EXECUTE_WRITECOPY: protection = "Execute/Write Copy";
	}

	strcpy_s(text, protection);
	if (guard)
		strcat_s(text, "/Guard");

	return text;
}

const CHAR* StateToString(DWORD state)
{
	switch (state) {
	case MEM_COMMIT: return "Committed";
	case MEM_RESERVE: return "Reserved";
	case MEM_FREE: return "Free";
	}
	return "";
}

const CHAR* MemoryTypeToString(DWORD type)
{
	switch (type) {
	case MEM_IMAGE: return "Image";
	case MEM_MAPPED: return "Mapped";
	case MEM_PRIVATE: return "Private";
	}

	return "";
}

const CHAR* GetExtraData(HANDLE hProcess, MEMORY_BASIC_INFORMATION& mbi)
{
	static CHAR text[512];
	if (mbi.Type == MEM_IMAGE)
	{
		if (::GetMappedFileNameA(hProcess, mbi.BaseAddress, text, sizeof(text)) > 0)
		{
			return text;
		}
	}
	return "";
}