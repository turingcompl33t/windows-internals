// InjectorLib.cpp
// Dll injection library implementation.

#include <windows.h>
#include <TlHelp32.h>

#include <iostream>

#include "InjectorLib.h"

#pragma warning(disable : 4703)       // possibly uninitialized value passed
									  // seems that code analysis not quite smart enough

/* ----------------------------------------------------------------------------
 *	Exported Methods
 */

INJECTORLIB_API Injector::Injector()
{
	DebugPrint("Initializing injector...");

	SupportedMethods = std::make_unique<std::vector<InjectionMethodInternal>>();
	SupportedMethods.get()->emplace_back(InjectionMethod::RemoteThreadSimple, "RemoteThreadSimple");
	SupportedMethods.get()->emplace_back(InjectionMethod::UserApcSimple, "UserApcSimple");

	std::cout << "[INJECTOR_DBG]: Loaded " << SupportedMethods.get()->size() << " injection methods\n";
}

InjectionStatus
INJECTORLIB_API
Injector::Inject(
	LPCSTR          hPayloadPath,
	DWORD           dwTargetPid,
	InjectionMethod method
)
{
	DebugPrint("Injection requested");

	auto status = InjectionStatus::StatusSuccess;

	switch (method)
	{
	case InjectionMethod::RemoteThreadSimple:
	{
		status = InjectViaRemoteThreadSimple(hPayloadPath, dwTargetPid);
		break;
	}
	case InjectionMethod::UserApcSimple:
	{
		status = InjectViaUserApcSimple(hPayloadPath, dwTargetPid);
		break;
	}
	default:
	{
		DebugPrint("Unrecognized injection method requested");
		break;
	}
	}

	return status;
}

std::unique_ptr<std::vector<std::string>>
INJECTORLIB_API
Injector::GetSupportedInjectionMethods()
{
	auto pVector = std::make_unique<std::vector<std::string>>();

	for (auto& m : *SupportedMethods.get())
	{
		pVector.get()->emplace_back(m.second);
	}

	return pVector;
}

InjectionMethod
INJECTORLIB_API
Injector::IndexToInjectionMethod(DWORD index)
{
	if (index >= SupportedMethods.get()->size())
	{
		return InjectionMethod::InvalidMethod;
	}

	return (*SupportedMethods.get())[index].first;
}

/* ----------------------------------------------------------------------------
 *	Private Methods
 */

// InjectViaRemoteThreadSimple
// Simplest possible DLL injection method; utilize
// VirtualAllocEx() / WriteProcessMemory() as the write
// primitive and transfer control via CreateRemoteThread()
//
// NOTE: specifies LoadLibraryA as the target of execution
// in the context of the remote process; this method will
// NOT work with raw shellcode written to remote process
//
// Arguments:
//	hPayloadPath - Path to the DLL to inject
//	dwTargetPid  - PID of the target process
InjectionStatus InjectViaRemoteThreadSimple(
	LPCSTR lpszPayloadPath,
	DWORD  dwTargetPid
)
{
	// TODO: potentially refactor this with smarter resource management
	// e.g. using WIL unique_handles (but VS does not seem to like these...)

	DebugPrint("Received injection request with parameters:");
	std::cout << "\tPayload Path:     " << lpszPayloadPath << '\n';
	std::cout << "\tTarget PID:       " << dwTargetPid << '\n';
	std::cout << "\tInjection Method: RemoteThreadSimple\n";


	HANDLE  hTargetProcess;
	HMODULE hKernel32;
	HANDLE  hRemoteThread;
	FARPROC pLoadLibrary = nullptr;
	LPVOID  pRemoteBase  = nullptr;

	auto status   = InjectionStatus::StatusSuccess;
	auto rollback = 0;

	// get a handle to the target process
	hTargetProcess = ::OpenProcess(
		PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
		FALSE,
		dwTargetPid
	);
	if (NULL == hTargetProcess)
	{
		DebugPrint("Failed to acquire handle to target process");
		status = InjectionStatus::StatusGeneralFailure;
		goto CLEANUP;
	}

	rollback = 1;

	// get the address of LoadLibrary()
	hKernel32 = ::GetModuleHandleA("Kernel32");
	if (NULL == hKernel32)
	{
		DebugPrint("Failed to acquire handle to Kernel32");
		status = InjectionStatus::StatusGeneralFailure;
		goto CLEANUP;
	}

	pLoadLibrary = ::GetProcAddress(hKernel32, "LoadLibraryA");
	if (NULL == pLoadLibrary)
	{
		DebugPrint("Failed to get address of LoadLibraryA()");
		status = InjectionStatus::StatusGeneralFailure;
		goto CLEANUP;
	}

	pRemoteBase = AllocateAndWriteProcessMemory(
		hTargetProcess,
		256,
		lpszPayloadPath,
		strlen(lpszPayloadPath) + 1
	);
	if (nullptr == pRemoteBase)
	{
		status = InjectionStatus::StatusGeneralFailure;
		goto CLEANUP;
	}

	// create remote thread in target process to load the library
	hRemoteThread = ::CreateRemoteThread(
		hTargetProcess,
		nullptr,
		0,
		reinterpret_cast<LPTHREAD_START_ROUTINE>(pLoadLibrary),
		pRemoteBase,
		0,
		nullptr
	);
	if (NULL == hRemoteThread)
	{
		DebugPrint("Failed to create remote thread in target process");
		status = InjectionStatus::StatusGeneralFailure;
		goto CLEANUP;
	}

	rollback = 2;

	DebugPrint("Successfully started remote thread in target process");

CLEANUP:	
	switch (rollback)
	{
	case 2:
		CloseHandle(hRemoteThread);
	case 1:
		CloseHandle(hTargetProcess);
	case 0:
		break;
	}

	return status;
}

// InjectViaUserApcSimple
// Slight variation on the CreateRemoteThread() injection
// method wherein control is transferred via user APC
// rather than direct remote thread creation; APC is 
// queued to each thread in target process, but execution
// is not guaranteed because the target process thread must
// enter an alertable wait state for APC to be executed
//
// NOTE: specifies LoadLibraryA as the target of execution
// in the context of the remote process; this method will
// NOT work with raw shellcode written to remote process
//
// Arguments:
//	hPayloadPath - Path to the DLL to inject
//	dwTargetPid  - PID of the target process
InjectionStatus InjectViaUserApcSimple(
	LPCSTR lpszPayloadPath,
	DWORD dwTargetPid
)
{
	DebugPrint("Received injection request with parameters:");
	std::cout << "\tPayload Path:     " << lpszPayloadPath << '\n';
	std::cout << "\tTarget PID:       " << dwTargetPid << '\n';
	std::cout << "\tInjection Method: UserApcSimple\n";

	HANDLE  hTargetProcess;
	HMODULE hKernel32;
	FARPROC pLoadLibrary = nullptr;
	LPVOID  pRemoteBase = nullptr;

	DWORD dwApcsQueued = 0;
	std::unique_ptr<std::vector<DWORD>> TargetProcessThreads;

	auto status = InjectionStatus::StatusSuccess;
	auto rollback = 0;

	// get a handle to the target process
	hTargetProcess = ::OpenProcess(
		PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
		FALSE,
		dwTargetPid
	);
	if (NULL == hTargetProcess)
	{
		DebugPrint("Failed to acquire handle to target process");
		status = InjectionStatus::StatusGeneralFailure;
		goto CLEANUP;
	}

	rollback = 1;

	// get the address of LoadLibrary()
	hKernel32 = ::GetModuleHandleA("Kernel32");
	if (NULL == hKernel32)
	{
		DebugPrint("Failed to acquire handle to Kernel32");
		status = InjectionStatus::StatusGeneralFailure;
		goto CLEANUP;
	}

	pLoadLibrary = ::GetProcAddress(hKernel32, "LoadLibraryA");
	if (NULL == pLoadLibrary)
	{
		DebugPrint("Failed to get address of LoadLibraryA()");
		status = InjectionStatus::StatusGeneralFailure;
		goto CLEANUP;
	}

	pRemoteBase = AllocateAndWriteProcessMemory(
		hTargetProcess,
		256,
		lpszPayloadPath,
		strlen(lpszPayloadPath) + 1
	);
	if (nullptr == pRemoteBase)
	{
		status = InjectionStatus::StatusGeneralFailure;
		goto CLEANUP;
	}

	// enumerate threads in the target process
	TargetProcessThreads = EnumerateProcessThreads(dwTargetPid);
	if (!TargetProcessThreads)
	{
		// enumeration failed; vector has already been released
		status = InjectionStatus::StatusGeneralFailure;
		goto CLEANUP;
	}

	std::cout << "[INJECTOR_DBG]: Enumerated " << TargetProcessThreads.get()->size() << " threads in target process\n";

	// queue a user APC to each thread in the target process
	for (const auto& tid : *TargetProcessThreads.get())
	{
		HANDLE hThread = ::OpenThread(THREAD_SET_CONTEXT, FALSE, tid);
		if (hThread != NULL)
		{
			::QueueUserAPC(
				reinterpret_cast<PAPCFUNC>(pLoadLibrary),
				hThread,
				reinterpret_cast<ULONG_PTR>(pRemoteBase)
			);
			::CloseHandle(hThread);
			dwApcsQueued++;
		}
	}
	
	std::cout << "[INJECTOR_DBG]: Successfully queued " << dwApcsQueued << " APC to target process threads\n";

CLEANUP:
	switch (rollback)
	{
	case 1:
		CloseHandle(hTargetProcess);
	case 0:
		break;
	}

	return status;
}

/* ----------------------------------------------------------------------------
 *	Internal Utilities
 */

// AllocateAndWriteProcessMemory
LPVOID AllocateAndWriteProcessMemory(
	HANDLE hTargetProcess,
	SIZE_T allocationSize,
	LPCSTR lpszPayload,
	SIZE_T payloadSize
	)
{
	LPVOID pRemoteBase = nullptr;

	// allocate storage in the target process
	pRemoteBase = ::VirtualAllocEx(
		hTargetProcess,
		nullptr,
		allocationSize,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE
	);
	if (NULL == pRemoteBase)
	{
		DebugPrint("Failed to allocate memory in target process");
		return nullptr;
	}

	// copy the name of the payload into the target process
	if (!::WriteProcessMemory(
		hTargetProcess,
		pRemoteBase,
		lpszPayload,
		payloadSize,
		nullptr
	))
	{
		DebugPrint("Failed to write payload path to target process memory");
		return nullptr;
	}

	return pRemoteBase;
}

std::unique_ptr<std::vector<DWORD>> EnumerateProcessThreads(
	DWORD dwPid
)
{
	HANDLE hSnapshot;
	THREADENTRY32 te = { sizeof(THREADENTRY32) };

	auto pVector = std::make_unique<std::vector<DWORD>>();

	hSnapshot = ::CreateToolhelp32Snapshot(
		TH32CS_SNAPTHREAD,
		dwPid
	);
	if (INVALID_HANDLE_VALUE == hSnapshot)
	{
		DebugPrint("Failed to create process snapshot");
		pVector.reset(nullptr);
		return pVector;
	}

	if (!::Thread32First(hSnapshot, &te))
	{
		DebugPrint("Failed to enumerate threads within snapshot");
		pVector.reset();
		return pVector;
	}

	do
	{
		if (te.th32OwnerProcessID == dwPid)
		{
			// if the thread belongs to the target process,
			// add it to the vector of thread IDs
			pVector.get()->push_back(te.th32ThreadID);
		}
	} while (::Thread32Next(hSnapshot, &te));

	return pVector;
}


void DebugPrint(const std::string_view msg)
{
#ifdef _DEBUG
	std::cout << "[INJECTOR DBG]: " << msg << '\n';
#endif
}