/*
 * GetProcAddress.cpp
 */

#include "GetProcAddress.h"

// acquire a handle to a module by name, in a specified target process
// NOTE: this function is already defined, but we rely on the fact that our 
// argument list is distinct in order to overload the existing function 
HINSTANCE GetModuleHandleEx(HANDLE hTargetProc, const TCHAR* lpModuleName)
{
	MODULEENTRY32 ME32{ 0 };
	ME32.dwSize = sizeof(ME32);

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(hTargetProc));
	if (INVALID_HANDLE_VALUE == hSnap)
	{
		// loop while the error is ERROR_BAD_LENGTH (see documentation)
		while (ERROR_BAD_LENGTH == GetLastError())
		{
			hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(hTargetProc));
			if (INVALID_HANDLE_VALUE != hSnap)
				break;
		}
	}

	// nothing we can do if we failed for another reason
	if (INVALID_HANDLE_VALUE == hSnap)
		return NULL;

	// search the snapshot for the target module
	BOOL bRet = Module32First(hSnap, &ME32);
	do
	{
		if (!_tcsicmp(lpModuleName, ME32.szModule))
			break;

		bRet = Module32Next(hSnap, &ME32);
	} while (bRet);

	CloseHandle(hSnap);

	if (!bRet)
		return NULL;

	// NOTE: HMODULE and HINSTANCE are equivalent types, so this is OK
	// QUESTION: why don't we return the module base address here?
	return ME32.hModule;
}

void* GetProcAddressEx(HANDLE hTargetProc, const TCHAR* lpModuleName, const char* lpProcName)
{
	// acquire handle to target module, reinterpret as byte address
	// QUESTION: why are we using the module handle here, instead of the base address?
	BYTE* modBase = reinterpret_cast<BYTE*>(GetModuleHandleEx(hTargetProc, lpModuleName));
	if (!modBase)
		return nullptr;

	// allocate local space for PE header of target module, read from target process
	BYTE* peHeader = new BYTE[0x1000];   // maximum size of PE header
	if (!peHeader)
		return nullptr;

	// read the PE header into our local buffer
	if (!ReadProcessMemory(hTargetProc, modBase, peHeader, 0x1000, nullptr))
	{
		delete[] peHeader;
		return nullptr;
	}

	// we can now access the various parts of the PE header because we have it mapped in local buffer
	// get a pointer to the NT header:
	//	extract the address of the start of the NT headers from the lfanew member of the DOS header
	//	add this to the base address (peHeader itself) to get to the start of the NT headers
	auto* pNT = reinterpret_cast<IMAGE_NT_HEADERS*>(peHeader + reinterpret_cast<IMAGE_DOS_HEADER*>(peHeader)->e_lfanew);
	// get a pointer to the export directory of NT header:
	//	access the final member of the optional header structure - the data directory
	//	index into the data directory to get the IMAGE_DATA_DIRECTORY_STRUCTURE itself
	//	use & to get address of this structure ( = VirtualAddress, Size)
	auto* pExportEntry = &pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (!pExportEntry->Size)
	{
		delete[] peHeader;
		return nullptr;
	}

	// allocate local space for the export data directory entry from target process module
	BYTE* exportData = new BYTE[pExportEntry->Size];
	if (!exportData)
	{
		delete[] peHeader;
		return nullptr;
	}

	// read the export data from the target process into our local buffer:
	//	extract the virtual address of the export data directory entry in the target image from the export entry
	//	add this virtual address (offset) to the module base to get the true virtual address of the export data
	if (!ReadProcessMemory(hTargetProc, modBase + pExportEntry->VirtualAddress, exportData, pExportEntry->Size, nullptr))
	{
		delete[] exportData;
		delete[] peHeader;
		return nullptr;
	}

	// calculate delta between base of export table in our process and in the target process
	BYTE* localBase = exportData - pExportEntry->VirtualAddress;
	// reinterpret the export directory data for easier access
	auto* pExportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(exportData);

	// set up a lambda to deal with potential forwarding of functions
	auto Forward = [&](DWORD FuncRVA) -> void*
	{
		char pFullExport[MAX_PATH + 1]{ 0 };
		auto Len = strlen(reinterpret_cast<char*>(localBase + FuncRVA));
		if (0 == Len)
			return nullptr;

		memcpy(pFullExport, reinterpret_cast<char*>(localBase + FuncRVA), Len);

		// forwarded function looks like e.g. "kernel32.ReadProcessMemory"
		// thus, this is the pattern we search for in the string we have read
		char* pFuncName = strchr(pFullExport, '.');
		// after execution of the above, pFuncName points at the "." in "kernel32.ReadProcessMemory"
		// replace the "." with NULL, and increment the pointer to beginning of the function name
		*(pFuncName++) = 0;

		// deal with functions being imported / exported by ordinal number, rather than name 
		if ('#' == *pFuncName)
			pFuncName = reinterpret_cast<char*>(LOWORD(atoi(++pFuncName)));

		// deal with unicode nonsense, and finally return the address of the procedure
#ifdef UNICODE
		TCHAR ModNameW[MAX_PATH + 1]{ 0 };
		size_t SizeOut = 0;
		mbstowcs_s(&SizeOut, ModNameW, pFullExport, MAX_PATH);
		
		return GetProcAddressEx(hTargetProc, ModNameW, pFuncName);
#else
		return GetProcAddressEx(hTargetProc, pFullExport, pFuncName);
#endif 
	};

	// determine if the lpProcName param is actual name or an ordinal number
	// ordinals may only range on [0, 0xFFFF], where 0xFFFF = MAXWORD
	if ((reinterpret_cast<UINT_PTR>(lpProcName) & 0xFFFFFF) <= MAXWORD)
	{
		// this must be an ordinal number, rather than proper function name

		WORD Base = LOWORD(pExportDir->Base - 1);   // not zero-indexed
		WORD Ordinal = LOWORD(lpProcName) - Base;   // extract the true ordinal number 

		DWORD FuncRVA = reinterpret_cast<DWORD*>(localBase + pExportDir->AddressOfFunctions)[Ordinal];

		delete[] exportData;
		delete[] peHeader;

		// if the function RVA points to something within the export directory, we know it is forwarded
		if (FuncRVA >= pExportEntry->VirtualAddress && FuncRVA < pExportEntry->VirtualAddress + pExportEntry->Size)
			return Forward(FuncRVA);

		// otherwise, it is not forwarded
		return modBase + FuncRVA;
	}

	// not an ordinal proc name, deal with true names here

	DWORD max     = pExportDir->NumberOfNames - 1;
	DWORD min     = 0;
	DWORD FuncRVA = 0;

	// do a binary search over the names in the export table
	// NOTE: the layout of the export table is strange, too complex to explain here in comments
	while (min < max)
	{
		DWORD mid = (min + max) / 2;

		DWORD CurrentRVA = reinterpret_cast<DWORD*>(localBase + pExportDir->AddressOfNames)[mid];
		char* szName = reinterpret_cast<char*>(localBase + CurrentRVA);

		int cmp = strcmp(szName, lpProcName);
		if (cmp < 0)
			min = mid + 1;
		else if (cmp > 0)
			max = mid - 1;
		else
		{
			// we found our proc!
			WORD Ordinal = reinterpret_cast<WORD*>(localBase + pExportDir->AddressOfNameOrdinals)[mid];
			FuncRVA = reinterpret_cast<DWORD*>(localBase + pExportDir->AddressOfFunctions)[Ordinal];
			break;
		}
	}

	delete[] exportData;
	delete[] peHeader;

	if (!FuncRVA)
		return nullptr;

	// again, if function RVA points to something within the export table, we know it is forwarded
	if (FuncRVA >= pExportEntry->VirtualAddress && FuncRVA < pExportEntry->VirtualAddress + pExportEntry->Size)
		return Forward(FuncRVA);

	// otherwise it is not forwarded, and we are good to go
	return modBase + FuncRVA;
}