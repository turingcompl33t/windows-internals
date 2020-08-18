/*
 * Source.cpp
 * Entry point for simple proof-of-concept application.
 */

#include <tchar.h>
#include <iostream>
#include <windows.h>

#include "LibDirectSyscalls.h"

VOID PrintSuccess(LPCSTR msg);
VOID PrintFailure(LPCSTR msg);

using namespace std;

/* ----------------------------------------------------------------------------
	POC Entry Point
*/

int _tmain(int argc, _TCHAR* argv[])
{
	LDS_STATUS status = LdsInit();
	if (LDS_STATUS_SUCCESS != status)
	{
		PrintFailure("Failed to initialize library. Exiting.");
		exit(99);
	}

	PrintSuccess("Library initialized successfully");

	HANDLE hThisProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
	if (INVALID_HANDLE_VALUE == hThisProcess)
	{
		PrintFailure("Failed to acquire handle to current process. Exiting");
		exit(99);
	}

	NTSTATUS retStatus;

	ULONG retSize;
	PROCESS_BASIC_INFORMATION processInfo;

	retStatus = LdsQueryInformationProcess(
		hThisProcess, 
		ProcessBasicInformation, 
		&processInfo, 
		sizeof(processInfo), 
		&retSize);

	if (NT_STATUS_SUCCESS != retStatus)
	{
		PrintFailure("QueryInformationProcess failed!");
		exit(99);
	}

	PrintSuccess("QueryInformationProcess succeeded!");
	printf("	PEB based address = %p\n", processInfo.PebBaseAddress);
	printf("	Unique process Identifier = %llu\n", processInfo.UniqueProcessId);

	cin.get();
	return 0;
}

/* ----------------------------------------------------------------------------
	Utility Functions
*/

VOID PrintSuccess(LPCSTR msg)
{
	cout << "[+] " << msg << endl;
}

VOID PrintFailure(LPCSTR msg)
{
	cout << "[-] " << msg << endl;
}