// RunSystem.cpp
// Execute arbitrary commands under NT AUTHORITY\SYSTEM.

#include <windows.h>
#include <WtsApi32.h>

#include <cstdio>
#include <iostream>
#include <string_view>

#pragma comment(lib, "wtsapi32")

/* ----------------------------------------------------------------------------
	Global Constants
 */

constexpr auto EXIT_SUCCESS_INTERNAL = 0x0;
constexpr auto EXIT_FAILURE_INTERNAL = 0x1;

constexpr auto BUFFER_SIZE = 512;

/* ----------------------------------------------------------------------------
	Function Prototypes
 */

HANDLE OpenSystemToken();
bool EnableCurrentProcessDebugPrivilege();
bool SetPrivilege(HANDLE hToken, const wchar_t* lpszPrivilege, bool bEnablePrivilege);
bool IsSystemSid(PSID pSid);

void LogInfo(const std::string_view msg);
void LogWarning(const std::string_view msg);
void LogError(const std::string_view msg);

/* ----------------------------------------------------------------------------
	Entry Point
 */

int wmain(int argc, wchar_t* argv[])
{
	if (argc < 2)
	{
		printf("[-] Error: invalid arguments\n");
		printf("[-] Usage: %ws <COMMAND LINE>\n", argv[0]);
		return EXIT_FAILURE_INTERNAL;
	}

	bool bRet;

	LogInfo("SysExec: Execute arbitrary commands under NT AUTHORITY\\SYSTEM");

	// enable debug privilege for the current process
	bRet = EnableCurrentProcessDebugPrivilege();
	if (!bRet)
	{
		LogWarning("Failed to enable debug privilege for the current process");
		LogWarning("Ensure you are running elevated and try again");
		return EXIT_FAILURE_INTERNAL;
	}

	LogInfo("Enabled debug privilege for current process");

	// acquire handle to a system token
	auto hSystemToken = OpenSystemToken();
	if (nullptr == hSystemToken)
	{
		LogWarning("Failed to open a system token");
		return EXIT_FAILURE_INTERNAL;
	}

	LogInfo("Opened a system token");

	HANDLE hTokenPrimary;
	HANDLE hTokenDup;
	
	// duplicate the system token to an impersonation token
	::DuplicateTokenEx(hSystemToken,
		TOKEN_DUPLICATE
		| TOKEN_IMPERSONATE
		| TOKEN_QUERY
		| TOKEN_ASSIGN_PRIMARY
		| TOKEN_ADJUST_PRIVILEGES, 
		nullptr,
		SecurityImpersonation, 
		TokenImpersonation, 
		&hTokenDup);
	
	// duplicate the system token to a primary token
	// only a primary token may be used in CreateProcessAsUser() call
	::DuplicateTokenEx(hSystemToken, 
		TOKEN_ALL_ACCESS, 
		nullptr, 
		SecurityImpersonation, 
		TokenPrimary, 
		&hTokenPrimary);

	// no need to keep the original system token around anymore
	::CloseHandle(hSystemToken);

	if (nullptr == hTokenDup || nullptr == hTokenPrimary)
	{
		LogWarning("Failed to duplicate system token");
		LogWarning("Ensure you are running elevated and try again");
		return EXIT_FAILURE_INTERNAL;
	}

	LogInfo("Duplicated the system token");

	std::wstring szCommandLine{};
	for (INT i = 1; i < argc; ++i)
	{
		szCommandLine += argv[i];
		szCommandLine += L" ";
	}

	STARTUPINFO si = { sizeof(STARTUPINFO) };
	si.lpDesktop = const_cast<wchar_t*>(L"Winsta0\\default");

	PROCESS_INFORMATION pi;

	// assign the impersonation system token to the executing thread
	bRet = ::SetThreadToken(nullptr, hTokenDup);
	if (!bRet)
	{
		LogWarning("Failed to SetThreadToken()");
		LogWarning("Ensure you are running elevated and try again");
		return EXIT_FAILURE_INTERNAL;
	}

	LogInfo("Set current thread token to system impersonation");

	HANDLE hCurrentToken;
	DWORD dwSessionId = 0;
	DWORD dwSessionIdLen = sizeof(dwSessionId);

	// get the session ID for the current process token
	::OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hCurrentToken);
	::GetTokenInformation(
		hCurrentToken, 
		TokenSessionId, 
		&dwSessionId, 
		dwSessionIdLen, 
		&dwSessionIdLen
		);
	::CloseHandle(hCurrentToken);

	// we require the following privileges to start the system process
	// SE_ASSIGNPRIMARYTOKEN_NAME - required to assign the primary token of a process
	// SE_INCREASE_QUOTA_NAME     - required to increase memory quota for a process
	if (!SetPrivilege(hTokenDup, SE_ASSIGNPRIMARYTOKEN_NAME, TRUE)
		|| !SetPrivilege(hTokenDup, SE_INCREASE_QUOTA_NAME, TRUE))
	{
		LogWarning("Failed to set privileges for impersonation token");
		return EXIT_FAILURE_INTERNAL;
	}

	LogInfo("Set privileges for the system impersonation token (under which we are executing)");

	// set the session ID for the primary token that will be used to create the process
	bRet = ::SetTokenInformation(hTokenPrimary, TokenSessionId, &dwSessionId, sizeof(dwSessionId));
	if (!bRet)
	{
		LogWarning("Failed to SetTokenInformation()");
		return EXIT_FAILURE_INTERNAL;
	}

	LogInfo("Set the session ID for the primary system token");



	// finally, do the thing
	bRet = CreateProcessAsUserW(hTokenPrimary, 
		nullptr, 
		const_cast<wchar_t*>(szCommandLine.c_str()), 
		nullptr, 
		nullptr, 
		FALSE, 
		0, 
		nullptr, 
		nullptr, 
		&si, 
		&pi);

	if (!bRet)
	{
		LogWarning("Failed to CreateProcessAsUser()");
	}
	else
	{
		LogInfo("System process started! Success!");
	}

	return EXIT_SUCCESS_INTERNAL;
}

/* ----------------------------------------------------------------------------
	Utiltities
 */

// find the first process with system SID and acquire handle to its token
HANDLE OpenSystemToken()
{	
	bool               bRet;
	unsigned long      dwCount;
	PWTS_PROCESS_INFO  pWtsProcessInfo;

	bRet = ::WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pWtsProcessInfo, &dwCount);
	if (!bRet)
	{
		LogWarning("Failed to WTSEnumerateProcesses()");
		return nullptr;
	}

	HANDLE hSystemToken{};
	for (unsigned i = 0; i < dwCount && !hSystemToken; ++i)
	{
		if (pWtsProcessInfo[i].SessionId == 0 &&
			IsSystemSid(pWtsProcessInfo[i].pUserSid))
		{
			auto hProcess = ::OpenProcess(
				PROCESS_QUERY_INFORMATION, 
				FALSE, 
				pWtsProcessInfo[i].ProcessId
				);

			if (INVALID_HANDLE_VALUE == hProcess)
			{
				// keep trying...
				continue;
			}

			// TOKEN_ASSIGN_PRIMARY - required to attach a primary token to a process
			// TOKEN_IMPERSONATE    - required to attach impersonation token to a process
			// TOKEN_DUPLICATE      - required to duplicate the token
			// TOKEN_QUERY          - required to query the token
			bRet = ::OpenProcessToken(hProcess, 
				TOKEN_ASSIGN_PRIMARY  
				| TOKEN_IMPERSONATE 
				| TOKEN_DUPLICATE 
				| TOKEN_QUERY, 
				&hSystemToken);

			::CloseHandle(hProcess);

			if (!bRet)
			{
				// keep trying...
				continue;
			}
		}
	}

	::WTSFreeMemory(pWtsProcessInfo);

	return hSystemToken;
}

// enable the debug privilege for the currently executing process
bool EnableCurrentProcessDebugPrivilege()
{
	bool   bRet;
	HANDLE hToken;

	bRet = ::OpenProcessToken(
		GetCurrentProcess(), 
		TOKEN_ADJUST_PRIVILEGES | 
		TOKEN_QUERY, 
		&hToken
		);

	if (!bRet)
	{
		LogWarning("Failed to OpenProcessToken()");
		return FALSE;
	}

	bRet = SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);
	
	::CloseHandle(hToken);

	return bRet;
}

// enable or disable specified privilege in the specified token
bool SetPrivilege(HANDLE hToken, const wchar_t* lpszPrivilege, bool bEnablePrivilege)
{
	bool             bRet;
	LUID             luid;
	TOKEN_PRIVILEGES TokenPrivileges;

	// lookup the local identifier for the specified privilege
	bRet = ::LookupPrivilegeValue(NULL, lpszPrivilege, &luid);
	if (!bRet)
	{
		LogWarning("Failed to LookupPrivilegeValue()");
		return FALSE;
	}

	// set up the token privileges structure 
	TokenPrivileges.PrivilegeCount = 1;
	TokenPrivileges.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
	{
		TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	}
	else
	{
		TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_REMOVED;
	}

	// adjust the token privileges accordingly
	// NOTE: thhis naive sizeof() here only works because we have a single privilege in the array
	bRet = ::AdjustTokenPrivileges(
		hToken, 
		FALSE, 
		&TokenPrivileges, 
		sizeof(TOKEN_PRIVILEGES), 
		nullptr, 
		nullptr
		);

	if (!bRet)
	{
		LogWarning("Failed to AdjustTokenPrivileges()");
		return FALSE;
	}

	if (ERROR_NOT_ALL_ASSIGNED == GetLastError())
	{
		LogWarning("Failed to AdjusTokenPrivileges()");
		return FALSE;
	}

	return TRUE;
}

// determine if the given SID is the LocalSystem SID
bool IsSystemSid(PSID pSid)
{
	return ::IsWellKnownSid(pSid, WinLocalSystemSid);
}

/* ----------------------------------------------------------------------------
	UI Output
*/

void LogInfo(const std::string_view msg)
{
	std::cout << "[+] " << msg << std::endl;
}

void LogWarning(const std::string_view msg)
{
	std::cout << "[-] " << msg << std::endl;
}

void LogError(const std::string_view msg)
{
	std::cout << "[!] " << msg << std::endl;
}