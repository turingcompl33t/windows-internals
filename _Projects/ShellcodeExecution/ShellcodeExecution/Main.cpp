/*
 * Main.cpp
 */

#include "StartRoutine.h"

#define DLL_PATH_X86 TEXT("C:\\Dev\\ShellcodeExecution\\Debug\\TestDll.dll")
#define DLL_PATH_X64 TEXT("C:\\Dev\\ShellcodeExecution\\Debug\\TestDll.dll")

#define PROCESS_NAME_X86 TEXT("TestTarget.exe")
#define PROCESS_NAME_X64 TEXT("TestTarget.exe")

#define LOAD_LIBRARY_NAME_A "LoadLibraryA"
#define LOAD_LIBRARY_NAME_W "LoadLibraryW"

#ifdef UNICODE
	#define LOAD_LIBRARY_NAME LOAD_LIBRARY_NAME_W
#else
	#define LOAD_LIBRARY_NAME LOAD_LIBRARY_NAME_A
#endif

#ifdef _WIN64
#define DLL_PATH DLL_PATH_X64
#define PROCESS_NAME PROCESS_NAME_X64
#else
#define DLL_PATH DLL_PATH_X86
#define PROCESS_NAME PROCESS_NAME_X86
#endif

BOOL InjectDll(const TCHAR* szProcess, const TCHAR* szPath, LAUNCH_METHOD Method);
HANDLE GetProcessByName(const TCHAR* szProcName, DWORD dwDesiredAccess);

void Error(const TCHAR* msg);

/* ----------------------------------------------------------------------------
	Entry Point
*/

int main()
{
	BOOL bRet = InjectDll(PROCESS_NAME, DLL_PATH, LM_QueueUserAPC);

	printf("Press Enter to Exit\n");
	std::cin.get();

	return 0;
}

/* ----------------------------------------------------------------------------
	Meat & Potatoes
*/

BOOL InjectDll(const TCHAR* szProcess, const TCHAR* szPath, LAUNCH_METHOD Method)
{
	// acquire a handle to the target process
	// NOTE: requires admin privilege because we request all access
	HANDLE hProc = GetProcessByName(szProcess, PROCESS_ALL_ACCESS);
	if (!hProc)
	{
		Error(TEXT("OpenProcess"));
		return FALSE;
	}

	// NOTE: safe method to compute string length, handling unicode and multibyte strings
	auto len = _tcslen(szPath) * sizeof(TCHAR);

	// allocate memory in the target process for the injectee DLL path
	void* pArg = VirtualAllocEx(hProc, nullptr, len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pArg)
	{
		Error(TEXT("VirtualAllocEx"));
		CloseHandle(hProc);
		return FALSE;
	}

	// write the path to injectee DLL into target process memory
	BOOL bRet = WriteProcessMemory(hProc, pArg, szPath, len, nullptr);
	if (!bRet)
	{
		Error(TEXT("WriteProcessMemory"));
		VirtualFreeEx(hProc, pArg, 0, MEM_RELEASE);
		CloseHandle(hProc);
		return FALSE;
	}

	// get address of the appropriate LoadLibrary function in the target process
	// reinterpret cast to f_Routine pointer (i.e. a pointer to a function pointer)
	f_Routine* pLoadLibrary = reinterpret_cast<f_Routine*>(GetProcAddressEx(hProc, TEXT("kernel32.dll"), LOAD_LIBRARY_NAME));
	if (!pLoadLibrary)
	{
		printf("Can't find LoadLibrary\n");
		VirtualFreeEx(hProc, pArg, 0, MEM_RELEASE);
		CloseHandle(hProc);
		return FALSE;
	}

	UINT_PTR hDllOut   = 0;
	DWORD    lastError = 0;

	// actually do the thing:
	//	hProc        - handle to target process 
	//	pLoadLibrary - address of the LoadLibrary function in target
	//	pArg         - address of DLL path written in target memory
	DWORD dwErr = StartRoutine(hProc, pLoadLibrary, pArg, Method, lastError, hDllOut);

	if (Method != LM_QueueUserAPC)
		VirtualFreeEx(hProc, pArg, 0, MEM_RELEASE);

	CloseHandle(hProc);

	if (dwErr)
	{
		Error(TEXT("StartRoutine"));
		printf("INTERNAL ERROR CODE: 0x%08x\n", dwErr);
		return FALSE;
	}

	printf("Success! LoadLibrary returned 0x%p\n", reinterpret_cast<void*>(hDllOut));

	return TRUE;
}

/* ----------------------------------------------------------------------------
	Helper Functions 
*/

// acquire a handle to a process by the name of the image executing within it
HANDLE GetProcessByName(const TCHAR* szProcName, DWORD dwDesiredAccess)
{
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnap)
		return nullptr;

	PROCESSENTRY32 PE32{ 0 };
	PE32.dwSize = sizeof(PE32);

	BOOL bRet = Process32First(hSnap, &PE32);
	while (bRet)
	{
		if (!_tcsicmp(PE32.szExeFile, szProcName))
			break;

		bRet = Process32Next(hSnap, &PE32);
	}

	CloseHandle(hSnap);

	if (!bRet)
		return nullptr;

	return OpenProcess(dwDesiredAccess, FALSE, PE32.th32ProcessID);
}

// helper function to print error information, error code
void Error(const TCHAR* msg)
{
	DWORD dwErr = GetLastError();
	_tprintf(TEXT("%s failed (0x%08x)\n"), msg, dwErr);
}