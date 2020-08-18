#pragma once

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <TlHelp32.h>

using f_LoadLibraryA   = HINSTANCE (WINAPI*)(const char* lpLibFilename);
using f_GetProcAddress = UINT_PTR  (WINAPI*)(HINSTANCE hModule, const char* lpProcName);
using f_DllEntryPoint  = BOOL      (WINAPI*)(void* hDll, DWORD dwReason, void* pReserved);

struct MANUAL_MAPPING_DATA
{
	f_LoadLibraryA    pLoadLibraryA;
	f_GetProcAddress  pGetProcAddress;
	HINSTANCE         hMod;
};

/*
 * ManualMap
 * Do the manual mapping.
 * 
 * Arguments:
 *	hProc     - handle to target process
 *	szDllFile - absolute path to dll to inject 
 *
 * Returns:
 *	TRUE on success
 *	FALSE on failure
 */
BOOL ManualMap(HANDLE hProc, const char* szDllFile);