// InjectorLib.h
// DLL injection library implementation.

#pragma once

#include <windows.h>

#include <vector>
#include <string_view>

#define INJECTORLIB_API __declspec(dllexport)

enum class InjectionMethod
{
	RemoteThreadSimple,
	UserApcSimple,
	InvalidMethod
};

enum class InjectionStatus
{
	StatusSuccess,
	StatusGeneralFailure
};

class Injector
{
public:
	INJECTORLIB_API Injector();

	InjectionStatus 
	INJECTORLIB_API
	Inject(
		LPCSTR          hPayloadPath, 
		DWORD           dwTargetPid, 
		InjectionMethod method
	);

	std::unique_ptr<std::vector<std::string>>
	INJECTORLIB_API
	GetSupportedInjectionMethods();

	InjectionMethod
	INJECTORLIB_API
	IndexToInjectionMethod(DWORD index);

private:
	// InjectionMethodInternal is the injection method
	// enumeration + the string representation of the method name
	using InjectionMethodInternal = std::pair<InjectionMethod, std::string>;

	std::unique_ptr<std::vector<InjectionMethodInternal>> SupportedMethods;
};

InjectionStatus InjectViaRemoteThreadSimple(
	LPCSTR lpszPayloadPath,
	DWORD  dwTargetPid
);

InjectionStatus InjectViaUserApcSimple(
	LPCSTR lpszPayloadPath,
	DWORD dwTargetPid
);

LPVOID AllocateAndWriteProcessMemory(
	HANDLE hTargetProcess,
	SIZE_T allocationSize,
	LPCSTR lpszPayload,
	SIZE_T payloadSize
);

std::unique_ptr<std::vector<DWORD>> EnumerateProcessThreads(
	DWORD  dwPid
);

void DebugPrint(const std::string_view msg);

