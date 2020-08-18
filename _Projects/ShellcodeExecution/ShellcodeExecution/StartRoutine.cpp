/*
 * StartRoutine.cpp
 */

#include "StartRoutine.h"

DWORD SR_NtCreateThreadEx (HANDLE hTargetProc, f_Routine* pRoutine, void* pArg, DWORD& LastWin32Error, UINT_PTR& RemoteRet);
DWORD SR_HijackThread     (HANDLE hTargetProc, f_Routine* pRoutine, void* pArg, DWORD& LastWin32Error, UINT_PTR& RemoteRet);
DWORD SR_SetWindowsHookEx (HANDLE hTargetProc, f_Routine* pRoutine, void* pArg, DWORD& LastWin32Error, UINT_PTR& RemoteRet);
DWORD SR_QueueUserAPC     (HANDLE hTargetProc, f_Routine* pRoutine, void* pArg, DWORD& LastWin32Error, UINT_PTR& RemoteRet);

// generic entry point for all injection / execution methods
DWORD StartRoutine(HANDLE hTargetProc, f_Routine* pRoutine, void *pArg, LAUNCH_METHOD Method, DWORD& LastWin32Error, UINT_PTR& RemoteRet)
{
	DWORD dwRet = 0;

	switch (Method)
	{
	case LM_NtCreateThreadEx:
		dwRet = SR_NtCreateThreadEx(hTargetProc, pRoutine, pArg, LastWin32Error, RemoteRet);
		break;
	case LM_HijackThread:
		dwRet = SR_HijackThread(hTargetProc, pRoutine, pArg, LastWin32Error, RemoteRet);
		break;
	case LM_SetWindowsHookEx:
		dwRet = SR_SetWindowsHookEx(hTargetProc, pRoutine, pArg, LastWin32Error, RemoteRet);
		break;
	case LM_QueueUserAPC:
		dwRet = SR_QueueUserAPC(hTargetProc, pRoutine, pArg, LastWin32Error, RemoteRet);
		break;
	default:
		dwRet = SR_ERR_INVALID_LAUNCH_METHOD;
		break;
	}
	return dwRet;
}

/*
 * SR_NtCreateThreadEx
 * Inject shellcode into the target process, and initiate execution of this shellcode by creating a remote thread.
 */
DWORD SR_NtCreateThreadEx(HANDLE hTargetProc, f_Routine* pRoutine, void* pArg, DWORD& LastWin32Error, UINT_PTR& RemoteRet)
{
	// get handle to ntdll module in the target process
	// get the address of NtCreateThread function in the target process
	// cast to approriate function pointer type
	auto p_NtCreateThreadEx = reinterpret_cast<f_NtCreateThread>(GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "NtCreateThreadEx"));
	if (!p_NtCreateThreadEx)
	{
		LastWin32Error = GetLastError();
		return SR_NTCTE_ERR_NTCTE_MISSING;
	}

	// allocate memory in the target process for shellcode (only required for x64)
	void* pmem = VirtualAllocEx(hTargetProc, nullptr, 0x100, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pmem)
	{
		LastWin32Error = GetLastError();
		return SR_NTCTE_ERR_CANT_ALLOC_MEM;
	}

#ifdef _WIN64
	// the shellcode that will be injected into our target process, only required for x64
	BYTE Shellcode[] =
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // - 0x10   -> argument / returned value
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // - 0x08   -> pRoutine

		0x48, 0x8B, 0xC1,                                   // + 0x00   -> mov rax, rcx                  move the pointer to dll path string into rax
		0x48, 0x8B, 0x08,                                   // + 0x03   -> mov rcx, [rax]                move the dll path string into rcx

		0x48, 0x83, 0xEC, 0x28,                             // + 0x06   -> sub rsp, 0x28                 allocate stack space for function call 
		0xFF, 0x50, 0x08,                                   // + 0x0A   -> call qword ptr [rax + 0x08]   call LoadLibrary, with dll path as argument 
		0x48, 0x83, 0xC4, 0x28,                             // + 0x0D   -> add rsp, 0x28                 clean up the stack 

		0x48, 0x8D, 0x0D, 0xD8, 0xFF, 0xFF, 0xFF,           // + 0x11   -> lea rcx, [pCodecave]          load the address of start of shellcode region into rcx
		0x48, 0x89, 0x01,                                   // + 0x18   -> mov [rcx], rax                move the return value of LoadLibrary to arg region
		0x48, 0x31, 0xC0,                                   // + 0x1B   -> xor rax, rax                  clear rax 

		0xC3                                                // + 0x1E   -> ret
	}; // SIZE = 0x1F (+ 0x10)

	// patch the shellcode buffer
	*reinterpret_cast<void**>      (Shellcode + 0x00) = pArg;          // insert pointer pArg in first 8 bytes of buffer
	*reinterpret_cast<f_Routine**> (Shellcode + 0x08) = pRoutine;      // insert pointer pRoutine in second 8 bytes of buffer

	DWORD FuncOffset = 0x10;   // offset into shellcode buffer where code actually begins

	// write the shellcode buffer into the memory we allocated in the target process
	BOOL bRet = WriteProcessMemory(hTargetProc, pmem, Shellcode, sizeof(Shellcode), nullptr);
	if (!bRet)
	{
		LastWin32Error = GetLastError();
		VirtualFreeEx(hTargetProc, pmem, 0, MEM_RELEASE);
		return SR_NTCTE_ERR_WPM_FAIL;
	}

	// grab a pointer to the remote argument, which is equivalent to the beginning of region where shellcode buffer was written
	void* pRemoteArg  = pmem;
	// grab a pointer to the remote function, which is the beginning of the code of portion of shellcode itself (not pRoutine)
	void* pRemoteFunc = reinterpret_cast<BYTE*>(pmem) + FuncOffset;

	HANDLE hThread = nullptr;

	// execute CreateThread in the target process; invoking shellcode with our remote argument as its argument
	NTSTATUS ntRet = p_NtCreateThreadEx(&hThread, THREAD_ALL_ACCESS, nullptr, hTargetProc, pRemoteFunc, pRemoteArg, 0, 0, 0, 0, nullptr);

	if (NT_FAIL(ntRet) || !hThread)
	{
		LastWin32Error = ntRet;
		VirtualFreeEx(hTargetProc, pmem, 0, MEM_RELEASE);
		return SR_NTCTE_ERR_NTCTE_FAIL;
	}

	// wait for the thread to exit
	DWORD dwWaitRet = WaitForSingleObject(hThread, SR_REMOTE_TIMEOUT);
	if (WAIT_OBJECT_0 != dwWaitRet)
	{
		LastWin32Error = GetLastError();
		TerminateThread(hThread, 0);
		CloseHandle(hThread);
		VirtualFreeEx(hTargetProc, pmem, 0, MEM_RELEASE);
		return SR_NTCTE_ERR_TIMEOUT;
	}

	CloseHandle(hThread);

	// read the return value from the remote buffer in target process
	// the return value is placed at the same location where the argument to shellcode was originally
	bRet = ReadProcessMemory(hTargetProc, pmem, &RemoteRet, sizeof(RemoteRet), nullptr);

	VirtualFreeEx(hTargetProc, pmem, 0, MEM_RELEASE);

	if (!bRet)
	{
		LastWin32Error = GetLastError();
		return SR_NTCTE_ERR_RPM_FAIL;
	}

#else
	HANDLE hThread = nullptr;

	// invoke CreateThread in the target process via the recovered pointer
	//	pRoutine - LoadLibrary entry point
	//	pArg     - path to injectee DLL
	NTSTATUS ntRet = p_NtCreateThreadEx(&hThread, THREAD_ALL_ACCESS, nullptr, hTargetProc, pRoutine, pArg, 0, 0, 0, 0, nullptr);

	if (NT_FAIL(ntRet) || !hThread)
	{
		LastWin32Error = ntRet;
		VirtualFreeEx(hTargetProc, pmem, 0, MEM_RELEASE);
		return SR_NTCTE_ERR_NTCTE_FAIL;
	}

	// wait for the thread to exit
	DWORD dwWaitRet = WaitForSingleObject(hThread, SR_REMOTE_TIMEOUT);
	if (WAIT_OBJECT_0 != dwWaitRet)
	{
		LastWin32Error = GetLastError();
		TerminateThread(hThread, 0);
		CloseHandle(hThread);
		VirtualFreeEx(hTargetProc, pmem, 0, MEM_RELEASE);
		return SR_NTCTE_ERR_TIMEOUT;
	}

	// grab the exit code information from the remote thread
	DWORD dwRemoteRet = 0;
	BOOL bRet = GetExitCodeThread(hThread, &dwRemoteRet);
	if (!bRet)
	{
		LastWin32Error = GetLastError();
		CloseHandle(hThread);
		return SR_NTCTE_ERR_RPM_FAIL;
	}

	RemoteRet = dwRemoteRet;
	CloseHandle(hThread);

#endif 

	return SR_ERR_SUCCESS;
}

/*
 * SR_HijackThread
 * Inject shellcode into the target process, and initiate execution of this shellcode by hijacking the thread 
 * context of an arbitrary thread executing within the target process.
 */
DWORD SR_HijackThread(HANDLE hTargetProc, f_Routine* pRoutine, void* pArg, DWORD& LastWin32Error, UINT_PTR& RemoteRet)
{
	THREADENTRY32 TH32{ 0 };
	TH32.dwSize = sizeof(TH32);

	// create a snapshot of all threads executing within the target process
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetProcessId(hTargetProc));
	if (INVALID_HANDLE_VALUE == hSnap)
	{
		LastWin32Error = GetLastError();
		return SR_HT_ERR_TH32_FAIL;
	}

	DWORD dwTargetProcId = GetProcessId(hTargetProc);
	DWORD threadId = 0;

	// grab the first thread in the snapshot
	BOOL bRet = Thread32First(hSnap, &TH32);
	if (!bRet)
	{
		LastWin32Error = GetLastError();
		CloseHandle(hSnap);
		return SR_HT_ERR_T32FIRST_FAIL;
	}

	// iterate over all threads until we identify one for which the owning process ID is that of the target process
	// QUESTION: why do we need to do this? aren't all threads in the snapshot executing within the target process?
	// NOTE: indeed, this is a quirk of the Toolhelp32 API
	do
	{
		if (TH32.th32OwnerProcessID == dwTargetProcId)
		{
			threadId = TH32.th32ThreadID;
			break;
		}

		bRet = Thread32Next(hSnap, &TH32);
	} while (bRet);

	// did we complete loop without satisfying the condition? fail
	if (!threadId)
		return SR_HT_ERR_NO_THREADS;

	// acquire a handle to the newly identified target thread
	// NOTE: the flags passed here to the OpenThread call are critical, we will be using all of them for hijacking
	HANDLE hThread = OpenThread(THREAD_SET_CONTEXT | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, threadId);
	if (!hThread)
	{
		LastWin32Error = GetLastError();
		return SR_HT_ERR_OPEN_THREAD_FAIL;
	}

	// suspend execution of the target thread
	// when we call SuspendThread, it returns the number of times it has been suspended, -1 on failure
	if (SuspendThread(hThread) == ((DWORD)-1))
	{
		LastWin32Error = GetLastError();
		CloseHandle(hThread);
		return SR_HT_ERR_SUSPEND_FAIL;
	}

	// setup the structure for the thread context, for future manipulation 
	CONTEXT oldContext{ 0 };
	oldContext.ContextFlags = CONTEXT_CONTROL;

	// query the current thread context of the target thread
	if (!GetThreadContext(hThread, &oldContext))
	{
		LastWin32Error = GetLastError();
		ResumeThread(hThread);
		CloseHandle(hThread);
		return SR_HT_ERR_GET_CONTEXT_FAIL;
	}

	// allocate memory region in the target process for our codecave
	void* pCodecave = VirtualAllocEx(hTargetProc, nullptr, 0x100, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pCodecave)
	{
		LastWin32Error = GetLastError();
		ResumeThread(hThread);
		CloseHandle(hThread);
		return SR_HT_ERR_CANT_ALLOC_MEM;
	}

#ifdef _WIN64

	BYTE Shellcode[] =
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                         // - 0x08           -> returned value

		0x48, 0x83, 0xEC, 0x08,                                                 // + 0x00           -> sub rsp, 0x08

		0xC7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00,                               // + 0x04 (+ 0x07)  -> mov [rsp], RipLowPart             move the current rip into 
		0xC7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00,                         // + 0x0B (+ 0x0F)  -> mov [rsp + 0x04], RipHighPart     top of stack location

		0x50, 0x51, 0x52, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53,       // + 0x13           -> push r(a/c/d)x / r(8 - 11)        save GPRs
		0x9C,                                                                   // + 0x1E           -> pushfq                            save flags registers

		0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             // + 0x1F (+ 0x21)  -> mov rax, pRoutine
		0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             // + 0x29 (+ 0x2B)  -> mov rcx, pArg                     prepare the argument

		0x48, 0x83, 0xEC, 0x20,                                                 // + 0x33           -> sub rsp, 0x20
		0xFF, 0xD0,                                                             // + 0x37           -> call rax                          call our routine
		0x48, 0x83, 0xC4, 0x20,                                                 // + 0x39           -> add rsp, 0x20

		0x48, 0x8D, 0x0D, 0xB4, 0xFF, 0xFF, 0xFF,                               // + 0x3D           -> lea rcx, [pCodecave]

		0x48, 0x89, 0x01,                                                       // + 0x44           -> mov [rcx], rax                    store the return value 

		0x9D,                                                                   // + 0x47           -> popfq                             restore flag registers
		0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5A, 0x59, 0x58,       // + 0x48           -> pop r(11-8) / r(d/c/a)x           restore GPRs

		0xC6, 0x05, 0xA9, 0xFF, 0xFF, 0xFF, 0x00,                               // + 0x53           -> mov byte ptr[$ - 0x57], 0         initialize check byte

		0xC3                                                                    // + 0x5A           -> ret
	}; // SIZE = 0x5B (+ 0x08)

	DWORD funcOffset = 0x08;                     // offset where shellcode actually begins in buffer
	DWORD checkbyteOffset = 0x03 + funcOffset;   // arbitrary offset for checkbyte location [$ - 0x57] (RIP relative addressing) 

	DWORD dwLoRip = (DWORD)(oldContext.Rip & 0xFFFFFFFF);                 // isolate the low doubleword of old RIP
	DWORD dwHiRip = (DWORD((oldContext.Rip) >> 0x20) & 0xFFFFFFFF);       // isolate the high doubleword of old RIP

	// patch the shellcode at runtime

	*reinterpret_cast<DWORD*>(Shellcode + 0x07 + funcOffset) = dwLoRip;   // insert low dword of RIP into shellcode
	*reinterpret_cast<DWORD*>(Shellcode + 0x0F + funcOffset) = dwHiRip;   // insert the high dword of RIP into shellcode

	*reinterpret_cast<void**>(Shellcode + 0x21 + funcOffset) = pRoutine;  // insert address of pRoutine into shellcode, for call 
	*reinterpret_cast<void**>(Shellcode + 0x28 + funcOffset) = pArg;      // insert address of pArg into shellcode, for call

	// update the instruction pointer of the thread context to begin execution at the start of our shellcode
	oldContext.Rip = reinterpret_cast<UINT_PTR>(pCodecave) + funcOffset;

#else 

	BYTE Shellcode[] =
	{
		0x00, 0x00, 0x00, 0x00,                     // - 0x04 (pCodecave)   -> returned value                          buffer to store returned value (eax)

		0x83, 0xEC, 0x04,                           // + 0x00               -> sub esp, 0x04                           prepare stack for ret
		0xC7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00,   // + 0x03 (+ 0x06)      -> mov [esp], OldEip                       store old eip as return address

		0x50, 0x51, 0x52,                           // + 0x0A               -> push e(a/c/d)                           save e(a/c/d)x
		0x9C,                                       // + 0x0D               -> pushfd                                  save flags register

		0xB9, 0x00, 0x00, 0x00, 0x00,               // + 0x0E (+ 0x0F)      -> mov ecx, pArg                           load pArg into ecx
		0xB8, 0x00, 0x00, 0x00, 0x00,               // + 0x13 (+ 0x14)      -> mov eax, pRoutine

		0x51,                                       // + 0x18               -> push ecx                                push pArg
		0xFF, 0xD0,                                 // + 0x19               -> call eax                                call target function

		0xA3, 0x00, 0x00, 0x00, 0x00,               // + 0x1B (+ 0x1C)      -> mov dword ptr[pCodecave], eax           store returned value

		0x9D,                                       // + 0x20               -> popfd                                   restore flags register
		0x5A, 0x59, 0x58,                           // + 0x21               -> pop e(d/c/a)                            restore e(d/c/a)x

		0xC6, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,   // + 0x24 (+ 0x26)      -> mov byte ptr[pCodecave + 0x06], 0x00    set checkbyte to 0

		0xC3                                       // + 0x2B                -> ret                                     return to OldEip

	}; // SIZE = 0x2C (+ 0x04)

	DWORD funcOffset = 0x04;                      // starting location of actual shellcode
	DWORD checkByteOffset = 0x02 + funcOffset;    // arbitrary location for storage of checkbyte [pCodecave + 0x06]

	// patch the shellcode at runtime

	*reinterpret_cast<DWORD*>(Shellcode + 0x06 + funcOffset) = oldContext.Eip;   // patch in the old instruction pointer 
	*reinterpret_cast<void**>(Shellcode + 0x0F + funcOffset) = pArg;             // patch in address of pArg, for call 
	*reinterpret_cast<void**>(Shellcode + 0x14 + funcOffset) = pRoutine;         // patch in the address of pRoutine, for call

	// patch in the address of beginning of codecave to store return value 
	*reinterpret_cast<void**>(Shellcode + 0x1C + funcOffset) = pCodecave;
	// patch in the address of the checkbyte
	*reinterpret_cast<BYTE**>(Shellcode + 0x26 + funcOffset) = reinterpret_cast<BYTE*>(pCodecave) + checkByteOffset;

	// set the instruction pointer of thread context to begin execution at start of our shellcode
	oldContext.Eip = reinterpret_cast<DWORD>(pCodecave) + funcOffset;

#endif

	// write our shellcode into the target process, in previously-allocated memory region
	if (!WriteProcessMemory(hTargetProc, pCodecave, Shellcode, sizeof(Shellcode), nullptr))
	{
		LastWin32Error = GetLastError();
		ResumeThread(hThread);
		CloseHandle(hThread);
		VirtualFreeEx(hTargetProc, pCodecave, 0, MEM_RELEASE);
		return SR_HT_ERR_WPM_FAIL;
	}

	// set the context of the target thread to the context we manipulated
	if (!SetThreadContext(hThread, &oldContext))
	{
		LastWin32Error = GetLastError();
		ResumeThread(hThread);
		CloseHandle(hThread);
		VirtualFreeEx(hTargetProc, pCodecave, 0, MEM_RELEASE);
		return SR_HT_ERR_SET_CONTEXT_FAIL;
	}

	// resume execution of the target thread
	if (ResumeThread(hThread) == ((DWORD)-1))
	{
		LastWin32Error = GetLastError();
		CloseHandle(hThread);
		VirtualFreeEx(hTargetProc, pCodecave, 0, MEM_RELEASE);
		return SR_HT_ERR_RESUME_FAIL;
	}

	CloseHandle(hThread);

	// monitor the status of the checkbyte in the buffer region of our shellcode
	// once the checkbyte has been reset (0) we know that our shellcode has completed execution

	DWORD timer    = GetTickCount();
	BYTE checkByte = 1;
	do
	{
		ReadProcessMemory(hTargetProc, reinterpret_cast<BYTE*>(pCodecave) + checkByteOffset, &checkByte, 1, nullptr);
		if (GetTickCount() - timer > SR_REMOTE_TIMEOUT)
			return SR_HT_ERR_TIMEOUT;
		Sleep(10);
	} while (checkByte);

	// read the return value of the routine we executed in the target process
	ReadProcessMemory(hTargetProc, pCodecave, &RemoteRet, sizeof(RemoteRet), nullptr);

	VirtualFreeEx(hTargetProc, pCodecave, 0, MEM_RELEASE);

	return SR_ERR_SUCCESS;
}

/*
 * SR_SetWindowsHookEx
 * Inject shellcode into a target process, and initiate execution of this shellcode by installing its
 * entry point as a window hook, courtesy of the easy-to-use API that MS has provided. 
 */
DWORD SR_SetWindowsHookEx(HANDLE hTargetProc, f_Routine* pRoutine, void* pArg, DWORD& LastWin32Error, UINT_PTR& RemoteRet)
{
	// allocate a memory region in the target process into which we will inject our shellcode
	void* pCodecave = VirtualAllocEx(hTargetProc, nullptr, 0x100, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pCodecave)
	{
		LastWin32Error = GetLastError();
		return SR_SWHEX_ERR_CANT_ALLOC_MEM;
	}

	// acquire a function point to the CallNextHookEx function, in the target VAS
	// we need this function because our hook will be installed a part of a hook chain, 
	// and thus we need to make sure we call the next hook in the chain when our hook completes 
	void* pCallNextHookEx = GetProcAddressEx(hTargetProc, TEXT("user32.dll"), "CallNextHookEx");
	if (!pCallNextHookEx)
	{
		VirtualFreeEx(hTargetProc, pCodecave, 0, MEM_RELEASE);
		LastWin32Error = GetLastError();
		return SR_SWHEX_ERR_CNHEX_MISSING;
	}

#ifdef _WIN64

	BYTE Shellcode[] =
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // - 0x18   -> pArg / returned value / rax  buffer
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // - 0x10   -> pRoutine                     pointer to target function
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // - 0x08   -> CallNextHookEx               pointer to CallNextHookEx

		0x55,                                           // + 0x00   -> push rbp                     save important registers
		0x54,                                           // + 0x01   -> push rsp
		0x53,                                           // + 0x02   -> push rbx

		0x48, 0x8D, 0x1D, 0xDE, 0xFF, 0xFF, 0xFF,       // + 0x03   -> lea rbx, [pArg]              load pointer into rbx

		0x48, 0x83, 0xEC, 0x20,                         // + 0x0A   -> sub rsp, 0x20                reserve stack
		0x4D, 0x8B, 0xC8,                               // + 0x0E   -> mov r9,r8                    set up arguments for CallNextHookEx
		0x4C, 0x8B, 0xC2,                               // + 0x11   -> mov r8, rdx
		0x48, 0x8B, 0xD1,                               // + 0x14   -> mov rdx,rcx
		0xFF, 0x53, 0x10,                               // + 0x17   -> call [rbx + 0x10]            call CallNextHookEx (patched in earlier)
		0x48, 0x83, 0xC4, 0x20,                         // + 0x1A   -> add rsp, 0x20                update stack

		0x48, 0x8B, 0xC8,                               // + 0x1E   -> mov rcx, rax                 copy retval into rcx

		0xEB, 0x00,                                     // + 0x21   -> jmp $ + 0x02                 jmp to next instruction
		0xC6, 0x05, 0xF8, 0xFF, 0xFF, 0xFF, 0x18,       // + 0x23   -> mov byte ptr[$ - 0x01], 0x1A hotpatch jmp above to skip shellcode

		0x48, 0x87, 0x0B,                               // + 0x2A   -> xchg [rbx], rcx              store CallNextHookEx retval, load pArg into rcx
		0x48, 0x83, 0xEC, 0x20,                         // + 0x2D   -> sub rsp, 0x20                reserve stack
		0xFF, 0x53, 0x08,                               // + 0x31   -> call [rbx + 0x08]            call pRoutine
		0x48, 0x83, 0xC4, 0x20,                         // + 0x34   -> add rsp, 0x20                update stack

		0x48, 0x87, 0x03,                               // + 0x38   -> xchg [rbx], rax              store pRoutine retval, restore CallNextHookEx retval

		0x5B,                                          // + 0x3B    -> pop rbx                      restore important registers
		0x5C,                                           // + 0x3C   -> pop rsp
		0x5D,                                           // + 0x3D   -> pop rbp

		0xC3                                            // + 0x3E   -> ret                          return
	}; // SIZE = 0x3F (+ 0x18)

	DWORD codeOffset      = 0x18;
	DWORD checkbyteOffset = 0x22 + codeOffset;

	*reinterpret_cast<void**>(Shellcode + 0x00) = pArg;              // patch in address of Arg
	*reinterpret_cast<void**>(Shellcode + 0x08) = pRoutine;          // patch in address of Routine 
	*reinterpret_cast<void**>(Shellcode + 0x10) = pCallNextHookEx;   // patch in address of CallNextHookEx

#else

	BYTE Shellcode[] =
	{
			0x00, 0x00, 0x00, 0x00,         // - 0x08               -> pArg                     pointer to argument
			0x00, 0x00, 0x00, 0x00,         // - 0x04               -> pRoutine                 pointer to target function

			0x55,                           // + 0x00               -> push ebp                 x86 stack frame creation
			0x8B, 0xEC,                     // + 0x01               -> mov ebp, esp

			0xFF, 0x75, 0x10,               // + 0x03               -> push [ebp + 0x10]        push CallNextHookEx arguments
			0xFF, 0x75, 0x0C,               // + 0x06               -> push [ebp + 0x0C]
			0xFF, 0x75, 0x08,               // + 0x09               -> push [ebp + 0x08]
			0x6A, 0x00,                     // + 0x0C               -> push 0x00

			0xE8, 0x00, 0x00, 0x00, 0x00,   // + 0x0E (+ 0x0F)      -> call CallNextHookEx      call CallNextHookEx

			0xEB, 0x00,                     // + 0x13               -> jmp $ + 0x02             jmp to next instruction

			0x50,                           // + 0x15               -> push eax                 save eax (CallNextHookEx retval)
			0x53,                           // + 0x16               -> push ebx                 save ebx (non volatile)

			0xBB, 0x00, 0x00, 0x00, 0x00,   // + 0x17 (+ 0x18)      -> mov ebx, pArg            move pArg (pCodecave) into ebx
			0xC6, 0x43, 0x1C, 0x14,         // + 0x1C               -> mov [ebx + 0x1C], 0x17   hotpatch jmp above to skip shellcode

			0xFF, 0x33,                     // + 0x20               -> push [ebx]               push pArg (__stdcall)

			0xFF, 0x53, 0x04,               // + 0x22               -> call [ebx + 0x04]        call target function

			0x89, 0x03,                     // + 0x25               -> mov [ebx], eax           store returned value

			0x5B,                           // + 0x27               -> pop ebx                  restore old ebx
			0x58,                           // + 0x28               -> pop eax                  restore eax (CallNextHookEx retval)
			0x5D,                           // + 0x29               -> pop ebp                  restore ebp
			0xC2, 0x0C, 0x00                // + 0x2A               -> ret 0x000C               return
	}; // SIZE = 0x3D (+ 0x08)

	DWORD codeOffset      = 0x08;
	DWORD checkbyteOffset = 0x14 + codeOffset;

	*reinterpret_cast<void**>(Shellcode + 0x00) = pArg;      // patch in the address of Arg 
	*reinterpret_cast<void**>(Shellcode + 0x04) = pRoutine;  // patch in the address of Routine

	// patch the relative call instruction with CallNextHookEx 
	// more complex here because we do not have access to RIP-relative addressing in x86
	*reinterpret_cast<DWORD*>(Shellcode + 0x0F + codeOffset) = reinterpret_cast<DWORD>(pCallNextHookEx) - reinterpret_cast<DWORD>(pCodecave) + 0x0E + codeOffset - 5;
	// patch in the address of Codecave
	*reinterpret_cast<void**>(Shellcode + 0x18 + codeOffset) = pCodecave;

#endif

	// write the shellcode into the memory region allocated in the target process
	if (!WriteProcessMemory(hTargetProc, pCodecave, Shellcode, sizeof(Shellcode), nullptr))
	{
		LastWin32Error = GetLastError();
		VirtualFreeEx(hTargetProc, pCodecave, 0, MEM_RELEASE);
		return SR_SWHEX_ERR_WPM_FAIL;
	}

	// make static for use in the lambda below 
	static EnumWindowsCallback_Data data;
	data.m_HookData.clear();

	// set the hook function to be entry point of shellcode
	data.m_pHook   = reinterpret_cast<HOOKPROC>(reinterpret_cast<BYTE*>(pCodecave) + codeOffset);
	data.m_PID     = GetProcessId(hTargetProc);
	data.m_hModule = GetModuleHandle(TEXT("user32.dll"));

	WNDENUMPROC enumWindowsCallback = [](HWND hWnd, LPARAM) -> BOOL
	{
		DWORD winPID = 0;
		// get the process and thread ID of the current window
		DWORD winTID = GetWindowThreadProcessId(hWnd, &winPID);
		if (winPID == data.m_PID)
		{
			// the PID of the window matches our target process PID
			TCHAR szWindow[MAX_PATH]{ 0 };
			if (IsWindowVisible(hWnd) && GetWindowText(hWnd, szWindow, MAX_PATH))
			{
				// the window is visible and the query for window text succeeded
				if (GetClassName(hWnd, szWindow, MAX_PATH) && _tcscmp(szWindow, TEXT("ConsoleWindowClass")))
				{
					// GetClassName succeeded and the window text is ConsoleWindowClass
					// (SetWindowsHookEx does not work on console windows)

					// finally, create the hook
					HHOOK hHook = SetWindowsHookEx(WH_CALLWNDPROC, data.m_pHook, data.m_hModule, winTID);

					// and if that succeeds, add it to our data structure
					if (hHook)
						data.m_HookData.push_back({ hHook, hWnd });
				}
			}
		}

		return TRUE;
	};

	// enumerate all windows, executing our callback for each enumerated
	if (!EnumWindows(enumWindowsCallback, reinterpret_cast<LPARAM>(&data)))
	{
		LastWin32Error = GetLastError();
		VirtualFreeEx(hTargetProc, pCodecave, 0, MEM_RELEASE);
		return SR_SWHEX_ERR_ENUM_WND_FAIL;
	}

	// check if at least one window hook was installed by enumeration + callback
	if (data.m_HookData.empty())
	{
		VirtualFreeEx(hTargetProc, pCodecave, 0, MEM_RELEASE);
		return SR_SWHEX_ERR_NO_WINDOWS;
	}

	HWND hForegroundWindow = GetForegroundWindow();

	// iterate over the hook data vector, forcing our hook to execute
	for (auto i : data.m_HookData)
	{
		SetForegroundWindow(i.m_hWnd);
		SendMessageA(i.m_hWnd, WM_KEYDOWN, VK_SPACE, 0);
		Sleep(10);
		SendMessageA(i.m_hWnd, WM_KEYUP, VK_SPACE, 0);
		UnhookWindowsHookEx(i.m_hHook);
	}

	// reset the foreground window to original value
	SetForegroundWindow(hForegroundWindow);

	DWORD timer = GetTickCount();
	BYTE checkByte = 0;

	// read target process memory to monitor checkbyte, ensure shellcode has executed
	do
	{
		ReadProcessMemory(hTargetProc, reinterpret_cast<BYTE*>(pCodecave) + checkbyteOffset, &checkByte, 1, nullptr);

		if (GetTickCount() - timer > SR_REMOTE_TIMEOUT)
			return SR_SWHEX_ERR_TIMEOUT;
		Sleep(10);
	} while (!checkByte);
	
	// get the return value of the routine injected into the target process
	ReadProcessMemory(hTargetProc, pCodecave, &RemoteRet, sizeof(RemoteRet), nullptr);

	VirtualFreeEx(hTargetProc, pCodecave, 0, MEM_RELEASE);

	return SR_ERR_SUCCESS;
}

/*
 * SR_QueueUserAPC
 * Inject shellcode into a target process, and initiate execution of this shellcode via a user APC. 
 *
 * NOTE: This method of shellcode execution relies on the APC mechanism defined by the operating system;
 * this implies that a thread in the target process must enter an alertable wait state AFTER our shellcode has
 * been injected (at which point the APC we have queued will be executed) in order for this to work.
 */
DWORD SR_QueueUserAPC(HANDLE hTargetProc, f_Routine* pRoutine, void* pArg, DWORD& LastWin32Error, UINT_PTR& RemoteRet)
{

	// allocate memory region in the target process for our codecave
	void* pCodecave = VirtualAllocEx(hTargetProc, nullptr, 0x100, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pCodecave)
	{
		LastWin32Error = GetLastError();
		return SR_QUAPC_ERR_CANT_ALLOC_MEM;
	}

#ifdef _WIN64

	BYTE Shellcode[] =
	{
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // - 0x18   -> returned value                         buffer to store returned value
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // - 0x10   -> pArg                                   buffer to store argument
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // - 0x08   -> pRoutine                               pointer to the rouinte to call

		0xEB, 0x00,                                         // + 0x00    -> jmp $+0x02                            jump to the next instruction

		0x48, 0x8B, 0x41, 0x10,                             // + 0x02   -> mov rax, [rcx + 0x10]                  move pRoutine into rax
		0x48, 0x8B, 0x49, 0x08,                             // + 0x06   -> mov rcx, [rcx + 0x08]                  move pArg into rcx

		0x48, 0x83, 0xEC, 0x28,                             // + 0x0A   -> sub rsp, 0x28                          reserve stack
		0xFF, 0xD0,                                         // + 0x0E   -> call rax                               call pRoutine
		0x48, 0x83, 0xC4, 0x28,                             // + 0x10   -> add rsp, 0x28                          update stack

		0x48, 0x85, 0xC0,                                   // + 0x14   -> test rax, rax                          check if rax indicates success/failure
		0x74, 0x11,                                         // + 0x17    -> je pCodecave + 0x2A                   jmp to ret if routine failed

		0x48, 0x8D, 0x0D, 0xC8, 0xFF, 0xFF, 0xFF,           // + 0x19   -> lea rcx, [pCodecave]                   load pointer to codecave into rcx
		0x48, 0x89, 0x01,                                   // + 0x20   -> mov [rcx], rax                         store returned value

		0xC6, 0x05, 0xD7, 0xFF, 0xFF, 0xFF, 0x28,           // + 0x23   -> mov byte ptr[pCodecave + 0x18], 0x28   hot patch jump to skip shellcode

		0xC3                                                // + 0x2A   -> ret                                    return
	}; // SIZE = 0x2B (+ 0x10)

	DWORD codeOffset = 0x18;

	// patch the shellcode at runtime

	*reinterpret_cast<void**>(Shellcode + 0x08) = pArg;       // patch in the address of pArg
	*reinterpret_cast<void**>(Shellcode + 0x10) = pRoutine;   // patch in the address of pRoutine

#else 

	BYTE Shellcode[] =
	{
		0x00, 0x00, 0x00, 0x00, // - 0x0C   -> returned value                   buffer to store returned value
		0x00, 0x00, 0x00, 0x00, // - 0x08   -> pArg                             buffer to store argument
		0x00, 0x00, 0x00, 0x00, // - 0x04   -> pRoutine                         pointer to the routine to call

		0x55,                   // + 0x00   -> push ebp                         x86 stack frame creation
		0x8B, 0xEC,             // + 0x01   -> mov ebp, esp

		0xEB, 0x00,             // + 0x03   -> jmp pCodecave + 0x05 (+ 0x0C)    jump to next instruction

		0x53,                   // + 0x05   -> push ebx                         save ebx
		0x8B, 0x5D, 0x08,       // + 0x06   -> mov ebx, [ebp + 0x08]            move pCodecave into ebx (non volatile)

		0xFF, 0x73, 0x04,       // + 0x09   -> push [ebx + 0x04]                push pArg on stack
		0xFF, 0x53, 0x08,       // + 0x0C   -> call dword ptr[ebx + 0x08]       call pRoutine

		0x85, 0xC0,             // + 0x0F   -> test eax, eax                    check if eax indicates success/failure
		0x74, 0x06,             // + 0x11   -> je pCodecave + 0x19 (+ 0x0C)     jmp to cleanup if routine failed

		0x89, 0x03,             // + 0x13   -> mov [ebx], eax                   store returned value
		0xC6, 0x43, 0x10, 0x15, // + 0x15   -> mov byte ptr [ebx + 0x10], 0x15  hot patch jmp to skip shellcode

		0x5B,                   // + 0x19   -> pop ebx                          restore old ebx
		0x5D,                   // + 0x1A   -> pop ebp                          restore ebp

		0xC2, 0x04, 0x00        // + 0x1B   -> ret 0x0004                       return
	}; // SIZE = 0x1E (+ 0x0C)

	DWORD codeOffset = 0x0C;

	// patch the shellcode at runtime

	*reinterpret_cast<void**>(Shellcode + 0x04) = pArg;      // patch in the address of pArg
	*reinterpret_cast<void**>(Shellcode + 0x08) = pRoutine;  // patch in the address of pRoutine

#endif

	// write our shellcode into the pre-allocated memory region in the target process
	BOOL bRet = WriteProcessMemory(hTargetProc, pCodecave, Shellcode, sizeof(Shellcode), nullptr);
	if (!bRet)
	{
		LastWin32Error = GetLastError();
		VirtualFreeEx(hTargetProc, pCodecave, 0, MEM_RELEASE);
		return SR_QUAPC_ERR_WPM_FAIL;
	}

	// get a snapshot of threads executing in the target process
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetProcessId(hTargetProc));
	if (INVALID_HANDLE_VALUE == hSnap)
	{
		LastWin32Error = GetLastError();
		VirtualFreeEx(hTargetProc, pCodecave, 0, MEM_RELEASE);
		return SR_QUAPC_ERR_TH32_FAIL;
	}

	DWORD targetPID = GetProcessId(hTargetProc);
	bool APCqueued = false;

	// the APC function that we will specify is the entry point of our shellcode
	PAPCFUNC pShellcode = reinterpret_cast<PAPCFUNC>(reinterpret_cast<BYTE*>(pCodecave) + codeOffset);

	THREADENTRY32 TH32;
	TH32.dwSize = sizeof(TH32);

	// get the first thread in the snapshot
	bRet = Thread32First(hSnap, &TH32);
	if (!bRet)
	{
		LastWin32Error = GetLastError();
		CloseHandle(hSnap);
		VirtualFreeEx(hTargetProc, pCodecave, 0, MEM_RELEASE);
		return SR_QUAPC_ERR_T32FIRST_FAIL;
	}

	// iterate over all threads in the snapshot until one is identified that is executing in our target process
	// NOTE: again, this is simply a quirk of this API that we have to do this at all
	do
	{
		if (TH32.th32OwnerProcessID == targetPID)
		{
			// acquire a handle to the target thread we have found
			HANDLE hThread = OpenThread(THREAD_SET_CONTEXT, FALSE, TH32.th32ThreadID);
			if (hThread)
			{
				// queue a user APC for the target thread, 
				// specifying the shellcode as the APC function and the entry to the codecave as the argument 
				// NOTE: this is not specifying pArg as the argument to the APC, we patched this into the shellcode itself
				if (QueueUserAPC(pShellcode, hThread, reinterpret_cast<ULONG_PTR>(pCodecave)))
				{
					APCqueued = true;
				} 
				else
				{
					LastWin32Error = GetLastError();
				}

				CloseHandle(hThread);
			}
		}

		bRet = Thread32Next(hSnap, &TH32);

	} while (bRet);

	CloseHandle(hSnap);

	if (!APCqueued)
	{
		// if APC was not queued, we failed
		VirtualFreeEx(hTargetProc, pCodecave, 0, MEM_RELEASE);
		return SR_QUAPC_ERR_NO_APC_THREAD;
	}
	else
	{
		// otherwise reset the error value
		LastWin32Error = 0;
	}

	DWORD timer = GetTickCount();
	RemoteRet = 0;

	// read checkbyte from process memory until we identify that shellcode has completed, or timeout
	do
	{
		ReadProcessMemory(hTargetProc, pCodecave, &RemoteRet, 1, nullptr);

		if (GetTickCount() - timer > SR_REMOTE_TIMEOUT)
			return SR_QUAPC_ERR_TIMEOUT;

		Sleep(10);

	} while (!RemoteRet);

	return SR_ERR_SUCCESS;
}
