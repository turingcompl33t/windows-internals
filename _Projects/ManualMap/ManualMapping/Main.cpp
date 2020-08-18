#include "Injection.h"

// the path to the dll we want to inject into process
const char szDllFile[]    = "Malicious.dll";
// the image executing in process we want to inject into
const char szTargetProc[] = "Target.exe";

void Error(const char* fn);

int main()
{

	PROCESSENTRY32 PE32{ 0 };              // simple structure representing a single process in snapshot
	PE32.dwSize = sizeof(PROCESSENTRY32);

	// create a snapshot of all processes currently in the system
	// second argument (0) is ignored when snapping processes
	// returns a handle to the snapshot object
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
		Error("CreateToolhelp32Snapshot");

	DWORD PID = 0;

	// returns true if the first process in snapshot extracted
	BOOL bRet = Process32First(hSnap, &PE32);   

	while (bRet)
	{
		// loop until we identify pid in which target image executes 
		if (!strcmp(szTargetProc, PE32.szExeFile))
		{
			PID = PE32.th32ParentProcessID;
			break;
		}
		// get the next process in the snapshot
		bRet = Process32Next(hSnap, &PE32);
	}

	CloseHandle(hSnap);

	// get a handle with full access to the target process
	// (is this really realistic?)
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
	if (NULL == hProc)
		Error("OpenProcess");
	
	// we have a handle to the target process, do the mapping
	if (!ManualMap(hProc, szDllFile))
	{
		CloseHandle(hProc);
		Error("ManualMap");
	}

	CloseHandle(hProc);

	return 0;
}

/* ----------------------------------------------------------------------------
	Helper Functions
*/

// error handling helper function
void Error(const char* fn)
{
	DWORD Err = GetLastError();
	printf("%s failed: 0x%X\n", fn, Err);
	system("PAUSE");
	exit(1);
}
