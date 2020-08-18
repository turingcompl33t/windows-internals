/*
 * StartRoutine.h
 */

#pragma once

#include <vector>
#include <iostream>

#include "GetProcAddress.h"

// function prototypes for function we will exec in target process
#ifdef _WIN64
using f_Routine = UINT_PTR(__fastcall*)(void* pArg);
#else
using f_Routine = UINT_PTR(__stdcall*)(void* pArg);
#endif 

enum LAUNCH_METHOD
{
	LM_NtCreateThreadEx, 
	LM_HijackThread, 
	LM_SetWindowsHookEx, 
	LM_QueueUserAPC
};

struct HookData
{
	HHOOK m_hHook;
	HWND  m_hWnd;
};

struct EnumWindowsCallback_Data
{
	std::vector<HookData> m_HookData;
	DWORD                 m_PID;
	HOOKPROC              m_pHook;
	HINSTANCE             m_hModule;
};

DWORD StartRoutine(
	HANDLE hTargetProc, 
	f_Routine* pRoutine, 
	void *pArg, 
	LAUNCH_METHOD Method, 
	DWORD& LastWin32Error, 
	UINT_PTR& RemoteRet);

using f_NtCreateThread = NTSTATUS(__stdcall*)(
	HANDLE* pThreadHandleOut, 
	ACCESS_MASK DesiredAccess, 
	void* pAttr, 
	HANDLE hProc, 
	void* pRoutine, 
	void* pArg, 
	ULONG Flags,
	SIZE_T ZeroBits, 
	SIZE_T StackSize, 
	SIZE_T MaxStackSize, 
	void* pAttrListOut);

#define SR_REMOTE_TIMEOUT               5000

#define SR_ERR_SUCCESS                  0x00000000
#define SR_ERR_INVALID_LAUNCH_METHOD    0x00000001

#define SR_NTCTE_ERR_NTCTE_MISSING      0x10000001
#define SR_NTCTE_ERR_CANT_ALLOC_MEM     0x10000002
#define SR_NTCTE_ERR_WPM_FAIL           0x10000003
#define SR_NTCTE_ERR_NTCTE_FAIL         0x10000004
#define SR_NTCTE_ERR_RPM_FAIL           0x10000005
#define SR_NTCTE_ERR_TIMEOUT            0x10000006

#define SR_HT_ERR_TH32_FAIL             0x20000001
#define SR_HT_ERR_T32FIRST_FAIL         0x20000002
#define SR_HT_ERR_NO_THREADS            0x20000003
#define SR_HT_ERR_OPEN_THREAD_FAIL      0x20000004
#define SR_HT_ERR_CANT_ALLOC_MEM        0x20000005
#define SR_HT_ERR_SUSPEND_FAIL          0x20000006
#define SR_HT_ERR_GET_CONTEXT_FAIL      0x20000007
#define SR_HT_ERR_WPM_FAIL              0x20000008
#define SR_HT_ERR_SET_CONTEXT_FAIL      0x20000009
#define SR_HT_ERR_RESUME_FAIL           0x2000000A
#define SR_HT_ERR_TIMEOUT               0x2000000B

#define SR_SWHEX_ERR_CANT_ALLOC_MEM     0x30000001
#define SR_SWHEX_ERR_CNHEX_MISSING      0x30000002
#define SR_SWHEX_ERR_WPM_FAIL           0x30000003
#define SR_SWHEX_ERR_ENUM_WND_FAIL      0x30000004
#define SR_SWHEX_ERR_NO_WINDOWS         0x30000005
#define SR_SWHEX_ERR_TIMEOUT            0x30000006

#define SR_QUAPC_ERR_CANT_ALLOC_MEM     0x40000001
#define SR_QUAPC_ERR_WPM_FAIL           0x40000002
#define SR_QUAPC_ERR_TH32_FAIL          0x40000003
#define SR_QUAPC_ERR_T32FIRST_FAIL      0x40000004
#define SR_QUAPC_ERR_NO_APC_THREAD      0x40000005
#define SR_QUAPC_ERR_TIMEOUT            0x40000006


#define NT_FAIL(status) (status < 0)