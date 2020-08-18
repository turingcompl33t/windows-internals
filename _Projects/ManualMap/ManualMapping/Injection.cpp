#include "Injection.h"

#define RELOC_FLAG32(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_HIGHLOW)
#define RELOC_FLAG64(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_DIR64)

#ifdef _WIN64
	#define RELOC_FLAG RELOC_FLAG64
#else
	#define RELOC_FLAG RELOC_FLAG32
#endif

void WINAPI GenShellcode(MANUAL_MAPPING_DATA* pData);
BYTE* VirtualAllocExHelper(HANDLE hProc, ULONGLONG ImageBase, DWORD SizeOfImage);

// actually do the thing
BOOL ManualMap(HANDLE hProc, const char* szDllFile)
{
	// setup to parse the PE file
	BYTE*                  pSrcData       = nullptr;
	IMAGE_NT_HEADERS*      pOldNtHeader   = nullptr;
	IMAGE_OPTIONAL_HEADER* pOldOptHeader  = nullptr;
	IMAGE_FILE_HEADER*     pOldFileHeader = nullptr;
	BYTE*                  pTargetBase    = nullptr;

	DWORD dwCheck = 0;
	if (INVALID_FILE_ATTRIBUTES == GetFileAttributesA(szDllFile))
	{
		printf("DLL file has invalid attributes\n");
		return FALSE;
	}

	std::ifstream File(szDllFile, std::ios::binary | std::ios::ate);

	if (File.fail())
	{
		printf("Opening the file failed: 0x%X\n", (DWORD)File.rdstate());
		File.close();
		return FALSE;
	}

	auto FileSize = File.tellg();
	if (FileSize < 0x1000)
	{
		printf("File size is invalid\n");
		File.close();
		return FALSE;
	}

	pSrcData = new BYTE[static_cast<UINT_PTR>(FileSize)];
	if (!pSrcData)
	{
		printf("Memory allocation failed\n");
		File.close();
		return FALSE;
	}

	// seek back to beginning
	File.seekg(0, std::ios::beg);

	// read the DLL file into memory
	File.read(reinterpret_cast<char*>(pSrcData), FileSize);
	
	// done with the DLL we want to inject
	File.close();

	if (reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_magic != 0x5A4D) // "MZ"
	{
		printf("DLL file has invalid format\n");
		delete[] pSrcData;
		return FALSE;
	}

	// extract header information from the image
	pOldNtHeader   = reinterpret_cast<IMAGE_NT_HEADERS*>(pSrcData + reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_lfanew);
	pOldOptHeader  = &pOldNtHeader->OptionalHeader;
	pOldFileHeader = &pOldNtHeader->FileHeader;

#ifdef _WIN64
	if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_AMD64)
	{
		printf("Invalid platform\n");
		delete[] pSrcData;
		return FALSE;
	}
#else
	if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
	{
		printf("Invalid platform\n");
		delete[] pSrcData;
		return FALSE;
	}
#endif 

	// try to allocate at the desired image base
	pTargetBase = VirtualAllocExHelper(hProc, pOldOptHeader->ImageBase, pOldOptHeader->SizeOfImage);

	if (!pTargetBase)
	{
		// try again without specifying the image base
		pTargetBase = VirtualAllocExHelper(hProc, 0, pOldOptHeader->SizeOfImage);

		if (!pTargetBase)
		{
			printf("Memory allocation failed (ex): 0x%X\n", GetLastError());
			delete[] pSrcData;
			return FALSE;
		}
	}

	MANUAL_MAPPING_DATA data{ 0 };
	data.pLoadLibraryA   = LoadLibraryA;
	data.pGetProcAddress = reinterpret_cast<f_GetProcAddress>(GetProcAddress);

	auto* pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeader);
	for(UINT i = 0; i != pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader)
	{
		// we only care about the sections that contain raw data
		// these are the ones that are initialized statically (prior to runtime)
		if (pSectionHeader->SizeOfRawData)
		{
			BOOL Res = WriteProcessMemory(
				hProc,
				pTargetBase + pSectionHeader->VirtualAddress,
				pSrcData + pSectionHeader->PointerToRawData,
				pSectionHeader->SizeOfRawData,
				nullptr
			);
			if (!Res)
			{
				printf("Can't map sections: 0x%X\n", GetLastError());
				delete[] pSrcData;
				VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
				return FALSE;
			}
		}
	}

	memcpy(pSrcData, &data, sizeof(data));
	WriteProcessMemory(hProc, pTargetBase, pSrcData, 0x1000, nullptr);

	delete[] pSrcData;

	// allocate some space for our shellcode
	void* pShellcode = VirtualAllocEx(hProc, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pShellcode)
	{
		printf("Memory allocation failed (ex): 0x%X\n", GetLastError());
		VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
		return FALSE;
	}

	WriteProcessMemory(hProc, pShellcode, GenShellcode, 0x1000, nullptr);

	HANDLE hThread = CreateRemoteThread(
		hProc, 
		nullptr, 
		0, 
		reinterpret_cast<LPTHREAD_START_ROUTINE>(pShellcode), 
		pTargetBase, 
		0, 
		nullptr
	);

	if (!hThread)
	{
		printf("CreateRemoteThread failed: 0x%X\n", GetLastError());
		VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
		VirtualFreeEx(hProc, pShellcode, 0, MEM_RELEASE);
		return FALSE;
	}

	CloseHandle(hThread);

	// ensure the shellcode has executed prior to deallocation
	HINSTANCE hCheck = NULL;
	while (!hCheck)
	{
		MANUAL_MAPPING_DATA data_checked{ 0 };
		ReadProcessMemory(hProc, pTargetBase, &data_checked, sizeof(data_checked), nullptr);
		hCheck = data_checked.hMod;
		Sleep(10);
	}

	VirtualFreeEx(hProc, pShellcode, 0, MEM_RELEASE);

	return TRUE;

}  // ManualMap

/*
 * Generate shellcode. 
 */
void WINAPI GenShellcode(MANUAL_MAPPING_DATA* pData)
{
	if (NULL == pData)
		return;

	BYTE* pBase = reinterpret_cast<BYTE*>(pData);
	auto* pOpt = &reinterpret_cast<IMAGE_NT_HEADERS*>(pBase + reinterpret_cast<IMAGE_DOS_HEADER*>(pData)->e_lfanew)->OptionalHeader;

	auto _LoadLibraryA     = pData->pLoadLibraryA;
	auto _GetProcAddress   = pData->pGetProcAddress;
	auto _DllMain          = reinterpret_cast<f_DllEntryPoint>(pBase + pOpt->AddressOfEntryPoint);

	// begin relocation process

	BYTE* LocationDelta = pBase + pOpt->ImageBase;
	if (LocationDelta)
	{
		// if no size information, can't perform relocation
		if (!pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
			return;

		auto* pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
		while (pRelocData->VirtualAddress)
		{
			UINT AmountOfEntries = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
			WORD* pRelativeInfo = reinterpret_cast<WORD*>(pRelocData + 1);
			for (UINT i = 0; i < AmountOfEntries; ++i, ++pRelativeInfo)
			{
				if (RELOC_FLAG(*pRelativeInfo))
				{
					UINT_PTR* pPatch = reinterpret_cast<UINT_PTR*>(pBase + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
					*pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);
				}
			}

			pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<BYTE*>(pRelocData) + pRelocData->SizeOfBlock);
		}
	}

	if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
	{
		auto* pImportDescriptor = 
			reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

		while (pImportDescriptor->Name)
		{
			char* szMod = reinterpret_cast<char*>(pBase + pImportDescriptor->Name);
			HINSTANCE hDll = _LoadLibraryA(szMod);
			ULONG_PTR* pThunkRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescriptor->OriginalFirstThunk);
			ULONG_PTR* pFuncRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescriptor->FirstThunk);

			if (!pThunkRef)
				pThunkRef = pFuncRef;

			for (; *pThunkRef; ++pThunkRef, ++pFuncRef)
			{
				// can load functions in two ways: name of function, or ordinal number
				if (IMAGE_SNAP_BY_ORDINAL(*pThunkRef))
				{
					*pFuncRef = _GetProcAddress(hDll, reinterpret_cast<char*>(*pThunkRef & 0xFFFF));
				} 
				else
				{
					auto* pImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pBase + (*pThunkRef));
					*pFuncRef = _GetProcAddress(hDll, pImport->Name);
				}
			}
			++pImportDescriptor;
		}
	}

	// thread local storage callbacks

	if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size)
	{
		auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
		auto* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);
		for (; pCallback && *pCallback; ++pCallback)
		{
			(*pCallback)(pBase, DLL_PROCESS_ATTACH, nullptr);
		}
	}

	// execute dll main
	_DllMain(pBase, DLL_PROCESS_ATTACH, nullptr);

	pData->hMod = reinterpret_cast<HINSTANCE>(pBase);

} // GenShellcode

/*
 * Wrapper for gross virtual allocation function.
 */
BYTE* VirtualAllocExHelper(HANDLE hProc, ULONGLONG ImageBase, DWORD SizeOfImage)
{
	return reinterpret_cast<BYTE*>(
		VirtualAllocEx(
			hProc,
			reinterpret_cast<void*>(ImageBase),
			SizeOfImage,
			MEM_COMMIT | MEM_RESERVE,
			PAGE_EXECUTE_READWRITE
		));
}