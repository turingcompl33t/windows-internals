/* 
 * The following ifdef block is the standard way of creating macros which make exporting
 * from a DLL simpler. All files within this DLL are compiled with the LIBDIRECTSYSCALLS_EXPORTS
 * symbol defined on the command line. This symbol should not be defined on any project
 * that uses this DLL. This way any other project whose source files include this file see
 * LIBDIRECTSYSCALLS_API functions as being imported from a DLL, whereas this DLL sees symbols
 * defined with this macro as being exported.
 */

#pragma once

#include <winternl.h>

#ifdef LIBDIRECTSYSCALLS_EXPORTS
#define LIBDIRECTSYSCALLS_API __declspec(dllexport)
#else
#define LIBDIRECTSYSCALLS_API __declspec(dllimport)
#endif

#define NT_STATUS_SUCCESS              0x00000000

#define LDS_STATUS_SUCCESS             0x00000000
#define LDS_STATUS_FAIL_INIT		   0x00000001
#define LDS_STATUS_FAIL_UNSUPPORTED_OS 0x00000002
#define LDS_STATUS_FAIL_UNINITIALIZED  0xFFFFFFFF

typedef struct _WIN_VER_INFO
{
	_TCHAR chOSMajorMinor[8];
} WIN_VER_INFO, *PWIN_VER_INFO;

// supported OS versions
enum class WinVersion {
	Unsupported,
	Win10
};

// defined by winternl.h, but not ptr version 
typedef CLIENT_ID* PCLIENT_ID;

typedef DWORD LDS_STATUS;

typedef NTSTATUS (NTAPI *_RtlGetVersion)(PRTL_OSVERSIONINFOEXW);

/* ----------------------------------------------------------------------------
 * Initialization
 */

LIBDIRECTSYSCALLS_API LDS_STATUS LdsInit(void);

/* ----------------------------------------------------------------------------
 * OpenProcess
 */

LIBDIRECTSYSCALLS_API NTSTATUS LdsOpenProcess(
	IN PHANDLE			  ProcessHandle, 
	IN ACCESS_MASK		  DesiredAccess, 
	IN POBJECT_ATTRIBUTES ObjectAttributes, 
	IN PCLIENT_ID		  ClientId
	);

EXTERN_C NTSTATUS NtOpenProcess10(
	IN PHANDLE		      ProcessHandle, 
	IN ACCESS_MASK		  DesiredAccess, 
	IN POBJECT_ATTRIBUTES ObjectAttributes, 
	IN PCLIENT_ID		  ClientId
	);

/* ----------------------------------------------------------------------------
 * ProtectVirtualMemory
 */

LIBDIRECTSYSCALLS_API NTSTATUS LdsProtectVirtualMemory(
	IN HANDLE  ProcessHandle,
	IN PVOID* BaseAddress,
	IN SIZE_T* NumberOfBytesToProtect,
	IN ULONG   NewAccessProtection,
	OUT PULONG OldAccessProtection
	);

EXTERN_C NTSTATUS NtProtectVirtualMemory(
	IN HANDLE  ProcessHandle, 
	IN PVOID*  BaseAddress, 
	IN SIZE_T* NumberOfBytesToProtect, 
	IN ULONG   NewAccessProtection, 
	OUT PULONG OldAccessProtection
	);

/* ----------------------------------------------------------------------------
 * WriteVirtualMemory
 */

LIBDIRECTSYSCALLS_API NTSTATUS LdsWriteVirtualMemory(
	IN HANDLE	hProcess,
	IN PVOID	lpBaseAddress,
	IN PVOID	lpBuffer,
	IN SIZE_T	NumberOfBytesToWrite,
	OUT PSIZE_T NumberOfBytesWritten
	);

EXTERN_C NTSTATUS NtWriteVirtualMemory(
	IN HANDLE	hProcess,
	IN PVOID	lpBaseAddress,
	IN PVOID	lpBuffer,
	IN SIZE_T	NumberOfBytesToWrite,
	OUT PSIZE_T NumberOfBytesWritten
);

/* ----------------------------------------------------------------------------
 * QueryInformationProcess
 */

LIBDIRECTSYSCALLS_API NTSTATUS LdsQueryInformationProcess(
	IN HANDLE           ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID           ProcessInformation,
	IN ULONG            ProcessInformationLength,
	OUT PULONG          ReturnLength
	);

EXTERN_C NTSTATUS NtQueryInformationProcess10(
	IN HANDLE			ProcessHandle, 
	IN PROCESSINFOCLASS ProcessInformationClass, 
	OUT PVOID			ProcessInformation, 
	IN ULONG			ProcessInformationLength, 
	OUT PULONG			ReturnLength
	);
