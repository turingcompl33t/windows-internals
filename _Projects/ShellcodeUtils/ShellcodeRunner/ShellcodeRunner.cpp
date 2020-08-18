// ShellcodeRunner.cpp
// Execute shellcode from file.
//
// Usage:
//	ShellcodeRunner.exe <INPUT FILE>
//
// Description:
//	INPUT_FILE - filesystem path to binary input file

#include <windows.h>

#include <iostream>
#include <string_view>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

using ShellcodeExec = void (*)();

void PrintInfo(const std::string_view msg);
void PrintWarning(const std::string_view msg);
void PrintUsage();

/* ----------------------------------------------------------------------------
 *	Program Entry
 */

int main(int argc, char* argv[])
{
	HANDLE        hInputFile;
	DWORD         dwFileSize;
	PBYTE         lpShellcodeBuffer;
	BOOL          bReadStatus;
	ShellcodeExec RunShellcode;

	bool InsertDebugBreak = false;
	auto status           = STATUS_SUCCESS_I;
	auto rollback         = 0;

	if (argc > 3)
	{
		PrintWarning("Invalid arguments");
		PrintUsage();
		return STATUS_FAILURE_I;
	}

	if (3 == argc)
	{
		if (0 == strcmp(argv[2], "/CC"))
		{
			InsertDebugBreak = true;
		}
		else
		{
			PrintWarning("Unrecognized arguments");
			PrintUsage();
			return STATUS_FAILURE_I;
		}
	}

	PrintInfo("Shellcode runner intitialized with parameters:");
	std::cout << "\tInput File:  " << argv[1] << '\n';
	std::cout << "\tDebug Break: " << (InsertDebugBreak ? "TRUE" : "FALSE") << '\n';

	// arguments are parsed; open the input file

	hInputFile = ::CreateFileA(
		argv[1],
		GENERIC_READ,
		0,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
	if (INVALID_HANDLE_VALUE == hInputFile)
	{
		PrintWarning("Failed to open input file");
		goto CLEANUP;
	}

	rollback = 1;

	// prepare shellcode buffer

	dwFileSize = ::GetFileSize(hInputFile, nullptr);
	if (NULL == dwFileSize)
	{
		PrintWarning("Failed to query input file size");
		goto CLEANUP;
	}

	lpShellcodeBuffer = static_cast<PBYTE>(
		::VirtualAlloc(
			0,
			static_cast<ULONG_PTR>(dwFileSize) + 1,
			MEM_COMMIT | MEM_RESERVE,
			PAGE_EXECUTE_READWRITE
		));
	if (NULL == lpShellcodeBuffer)
	{
		PrintWarning("Failed to allocate memory region for shellcode");
		goto CLEANUP;
	}

	// read shellcode from file into buffer

	if (InsertDebugBreak)
	{
		*lpShellcodeBuffer = 0xCC;
		bReadStatus = ::ReadFile(
			hInputFile,
			static_cast<LPVOID>(lpShellcodeBuffer+1),
			dwFileSize,
			nullptr,
			nullptr
		);
	}
	else
	{
		bReadStatus = ::ReadFile(
			hInputFile,
			static_cast<LPVOID>(lpShellcodeBuffer),
			dwFileSize,
			nullptr,
			nullptr
		);
	}

	if (!bReadStatus)
	{
		PrintWarning("Failed to read shellcode into buffer");
		goto CLEANUP;
	}

	// execute shellcode

	RunShellcode = reinterpret_cast<ShellcodeExec>(lpShellcodeBuffer);
	
	RunShellcode();

	PrintInfo("Shellcode execution complete");

CLEANUP:
	switch (rollback)
	{
	case 1:
		::CloseHandle(hInputFile);
	case 0:
		break;
	}

	return status;
}

/* ---------------------------------------------------------------------------- 
 *	Utility Functions
 */

void PrintInfo(const std::string_view msg)
{
	std::cout << "[+] " << msg << '\n';
}

void PrintWarning(const std::string_view msg)
{
	std::cout << "[-] " << msg << '\n';
}

void PrintUsage()
{
	std::cout << "[-] ShellcodeRunner.exe <FILE PATH> [/CC]\n";
}