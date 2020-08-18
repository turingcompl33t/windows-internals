// RunAppContainer.cpp
// Run arbitrary programs within an AppContainer.

#include <windows.h>
#include <UserEnv.h>

#include <iostream>

#pragma comment(lib, "userenv")

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

bool RunAppContainer(
	const std::wstring& szAppContainerName, 
	const std::wstring& szExectuablePath
	);

void LogInfo(const std::string& msg);
void LogWarning(const std::string& msg);
void LogError(const std::string& msg);

/* ----------------------------------------------------------------------------
	Entry Point
 */

int wmain(int argc, wchar_t* argv[])
{
	// weak argument validation
	if (3 != argc)
	{
		printf("[-] Error: invalid arguments\n");
		printf("[-] Usage: %ws <CONTAINER NAME> <EXECUTABLE PATH>\n", argv[0]);
		return STATUS_FAILURE_I;
	}

	LogInfo("RunAppContainer - run arbitrary executables within an AppContainer");
	printf("[+] Received AppContainer name: %ws\n", argv[1]);
	printf("[+] Received executable path:   %ws\n", argv[2]);

	if (!RunAppContainer(argv[1], argv[2]))
	{
		LogWarning("Failed to launch AppContainer");
	}

	return STATUS_SUCCESS_I;
}

/* ----------------------------------------------------------------------------
	RunAppContainer
 */

bool RunAppContainer(
	const std::wstring& szAppContainerName, 
	const std::wstring& szExecutablePath
	)
{
	bool    bRes;
	HRESULT hRes;
	PSID    pContainerSid;

	hRes = ::CreateAppContainerProfile(
		szAppContainerName.c_str(),
		szAppContainerName.c_str(),
		szAppContainerName.c_str(),
		nullptr,
		0,
		&pContainerSid
		);

	if (S_OK != hRes
		&& HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hRes)
	{
		// profile creation failed because the name already exists
		// attempt to derive the SID from the existing container
		hRes = ::DeriveAppContainerSidFromAppContainerName(
			szAppContainerName.c_str(), 
			&pContainerSid
			);
	}

	if (S_OK != hRes)
	{
		LogWarning("Failed to get AppContainer SID for specified name");
		return FALSE;
	}

	// profile exists or creation succeeded; proceed to process creation

	SIZE_T                AttributesSize;
	PROCESS_INFORMATION   pi;
	SECURITY_CAPABILITIES sc = { 0 };
	STARTUPINFOEX         si = { sizeof(STARTUPINFOEX) };

	sc.AppContainerSid = pContainerSid;

	// determine the size of the buffer required for attributes
	bRes = ::InitializeProcThreadAttributeList(nullptr, 1, 0, &AttributesSize);

	auto buffer = std::make_unique<BYTE[]>(AttributesSize);
	si.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(buffer.get());

	bRes = ::InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &AttributesSize);
	if (!bRes)
	{
		LogWarning("Failed to InitializeProcThreadAttributeList()");
		return FALSE;
	}

	// specify that we want to create the new process as an AppContainer
	// NOTE: we specify 0 capabilities for the token, for now
	bRes = ::UpdateProcThreadAttribute(
		si.lpAttributeList, 
		0, 
		PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES, 
		&sc, 
		sizeof(sc), 
		nullptr, 
		nullptr
		);

	if (!bRes)
	{
		LogWarning("Failed to UpdateProcThreadAttributes()");
		return FALSE;
	}
	
	const size_t len = wcslen(szExecutablePath.c_str()) + 1;
	auto cmdline     = std::make_unique<WCHAR[]>(len);

	wcscpy_s(cmdline.get(), len, szExecutablePath.c_str());

	bRes = ::CreateProcessW(
		nullptr, 
		cmdline.get(), 
		nullptr, 
		nullptr, 
		FALSE, 
		EXTENDED_STARTUPINFO_PRESENT, 
		nullptr, 
		nullptr, 
		(LPSTARTUPINFO)&si, 
		&pi
		);

	if (!bRes)
	{
		LogWarning("Failed to CreateProcess()");
		return FALSE;
	}

	LogInfo("AppContainer launched successfully");

	// wait for process to exit
	::WaitForSingleObject(pi.hProcess, INFINITE);

	// start cleanup
	::CloseHandle(pi.hThread);
	::CloseHandle(pi.hProcess);

	::LocalFree(pContainerSid);

	// TODO: what would happen here if we deleted the profile 
	// while the container continued to execute? 
	// that is, if we did not wait for child process to exit
	// prior to reaching this point and destroying profile
	hRes = ::DeleteAppContainerProfile(szAppContainerName.c_str());
	if (S_OK != hRes)
	{
		LogWarning("Failed to DeleteAppContainerProfile()");
	}

	return TRUE;
}

/* ---------------------------------------------------------------------------- 
	Utility Functions
 */

VOID LogInfo(const std::string& msg)
{
	std::cout << "[+] " << msg << std::endl;
}

VOID LogWarning(const std::string& msg)
{
	std::cout << "[-] " << msg << std::endl;
}

VOID LogError(const std::string& msg)
{
	std::cout << "[!] " << msg << std::endl;
}
