// ShellcodeWriter.cpp
// Assemble input shellcode assembly files (via MASM) and dump to raw shellcode file.
//
// Usage:
//	ShellcodeWriter.exe <INPUT PATH> <OUTPUT PATH>
//
// Description:
//	INPUT_PATH  - filesystem path to x64 assembly source file (.asm)
//	OUTPUT_PATH - filesystem path to binary output file

#include <windows.h>

#include <string>
#include <iostream>
#include <string_view>

#include <PeLib.h>

#pragma comment(lib, "PeLib.lib")

#pragma warning(disable : 4703)     // potentially uninitialized variable used

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr auto CMDLINE_BUFFER_SIZE = 512;

using TextInfo = std::pair<std::unique_ptr<unsigned char[]>, size_t>;

TextInfo ExtractShellcodeFromExecutable(const std::string& path);

void PrintInfo(const std::string_view msg);
void PrintWarning(const std::string_view msg);
void PrintUsage();

/* ----------------------------------------------------------------------------
 *	Program Entry
 */

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		PrintWarning("Invalid arguments");
		PrintUsage();
		return STATUS_FAILURE_I;
	}

	HANDLE   hOutfile;
	DWORD    dwBytesWritten;
	DWORD    dwExitCode;
	TextInfo info;

	STARTUPINFOA si = { sizeof(STARTUPINFOA) };
	PROCESS_INFORMATION pi;

	auto status = STATUS_SUCCESS_I;
	auto rollback = 0;

	// prepare the command line

	CHAR commandLine[CMDLINE_BUFFER_SIZE];
	sprintf_s(commandLine, "ml64.exe %s /FeTemp.exe /link /entry:main /nodefaultlib", argv[1]);

	// assemble the input file

	if (!::CreateProcessA(
		nullptr,
		commandLine,
		nullptr,
		nullptr,
		FALSE,
		0,
		nullptr,
		nullptr,
		&si,
		&pi
	))
	{
		PrintWarning("Failed to create assembler process");
		status = STATUS_FAILURE_I;
		goto CLEANUP;
	}

	rollback = 1;
	PrintInfo("Started assembler process; waiting for completion...");

	::WaitForSingleObject(pi.hProcess, INFINITE);

	::GetExitCodeProcess(pi.hProcess, &dwExitCode);
	if (dwExitCode != 0)
	{
		PrintWarning("Assembly completed with nonzero exit code; aborting");
		goto CLEANUP;
	}

	rollback = 2;
	PrintInfo("Assembly completed successfully; dumping text...");

	// read shellcode from assembled executable
	info = ExtractShellcodeFromExecutable("Temp.exe");
	if (!info.first)
	{
		goto CLEANUP;
	}

	PrintInfo("Successully extracted shellcode from executable");

	// dump shellcode to output file
	hOutfile = ::CreateFileA(
		argv[2],
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
	if (INVALID_HANDLE_VALUE == hOutfile)
	{
		PrintWarning("Failed to open output file");
		goto CLEANUP;
	}

	rollback = 3;

	if (!::WriteFile(
		hOutfile,
		info.first.get(),
		info.second,
		&dwBytesWritten,
		nullptr
	))
	{
		PrintWarning("Failed to write shellcode");
		goto CLEANUP;
	}

	if (dwBytesWritten != info.second)
	{
		std::cout << "[-] Failed to write complete shellcode content (wrote " << dwBytesWritten << " bytes)\n";
	}
	else
	{
		std::cout << "[+] Successfully wrote " << dwBytesWritten << " shellcode bytes\n";
	}

CLEANUP:
	switch (rollback)
	{
	case 3:
		::CloseHandle(hOutfile);
	case 2:
		// remove the temporary file
		::DeleteFileA("Temp.exe");
	case 1:
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	case 0:
		break;
	}

	return status;
}

/* ----------------------------------------------------------------------------
 *	Supporting Functions
 */

// ExtractShellcodeFromExecutable
// Parse temporary executable produced by MASM and extract
// the .text section to dynamically allocated buffer.
//
// Arguments:
//	path - path to executable file
//
// NOTE: allocates buffer; caller responsible for free
TextInfo ExtractShellcodeFromExecutable(const std::string& path)
{
	PeLib::PeParser pe{ path };

	if (pe.parse() != PeLib::PeParseResult::ResultSuccess)
	{
		PrintWarning("Failed to parse executable");
		return TextInfo{ nullptr, 0 };
	}

	auto section = pe.get_section_by_name(".text");
	if (!section)
	{
		PrintWarning("Failed to locate executable .text section");
		return TextInfo{ nullptr, 0 };
	}

	size_t size = section->Header.VirtualSize;
	auto lpShellcodeBuffer = std::make_unique<unsigned char[]>(section->Header.VirtualSize);

	// read contents of .text section through VirtualSize bytes
	memcpy_s(lpShellcodeBuffer.get(), size, (section->Content).get(), size);

	return TextInfo{ std::move(lpShellcodeBuffer), size };
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
	std::cout << "[-] ShellcodeWriter.exe <INPUT PATH> <OUTPUT PATH>\n";
}