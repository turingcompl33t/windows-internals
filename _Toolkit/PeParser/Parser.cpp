/*
 * Parser.cpp
 * Simple PE32 / PE32+ binary format parser. 0027B2A8
 */

#include <cstdio>
#include <iostream>
#include <Windows.h>

#include "Parser.h"

BOOL SanityCheck(LPBYTE pBuffer);
void ParseDOSHeader(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData);
void ParseNTHeaders(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData);
void ParseFileHeader(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData);
void ParseOptionalHeader(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData);
void ParseOptionalHeader32(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData);
void ParseOptionalHeader64(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData);
void ParseDataDirectories(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData);
void ParseDataDirectory(IMAGE_DATA_DIRECTORY* pImageDataDir, PPE_IMAGE_DATA pImageData, BYTE index);
void PrintDataDirectoryName(BYTE index);
void ParseSectionHeaders(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData);

void ParseImportDirectory(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData);
void ParseDebugDirectory(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData);
ULONG RvaToOffset(DWORD va, PPE_IMAGE_DATA pImageData);

void PrintUsage();
void PrintError(const char* msg);
void PrintCharacteristic(DWORD dCharacteristics, DWORD dMask, const char* str);

int main(int argc, char *argv[])
{
	if (2 != argc)
	{
		PrintError("invalid arguments");
		PrintUsage();
		return 1;
	}

	HANDLE hFile;
	DWORD  dFileSize;
	BOOL   bRet;
	DWORD  dBytesRead;
	LPBYTE pBuffer = NULL;

	PE_IMAGE_DATA ImageData{ 0 };

	LPCSTR filename = argv[1];

	// acquire a handle to the binary file
	hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		PrintError("failed to open file");
		goto EXIT;
	}

	// get the file size
	dFileSize = GetFileSize(hFile, NULL);
	if (INVALID_FILE_SIZE == dFileSize)
	{
		PrintError("invalid file size");
		goto EXIT;
	}

	// allocate space into which file will be read
	pBuffer = new BYTE[static_cast<UINT_PTR>(dFileSize)]; 
	if (NULL == pBuffer)
	{
		PrintError("failed to allocate file buffer");
		goto EXIT;
	}

	// read the file into memory
	bRet = ReadFile(hFile, pBuffer, dFileSize, &dBytesRead, NULL);
	if (FALSE == bRet)
	{
		printf("failed to read file into buffer");
		goto EXIT;
	}

	// good to start parsing
	printf("\n[+] Parsing file: %s\n", filename);
	if (!SanityCheck(pBuffer))
	{
		PrintError("file format is invalid - not PE32 / PE32+");
		goto EXIT;
	}

	ParseDOSHeader(pBuffer, &ImageData);
	ParseNTHeaders(pBuffer, &ImageData);
	ParseFileHeader(pBuffer, &ImageData);
	ParseOptionalHeader(pBuffer, &ImageData);
	ParseDataDirectories(pBuffer, &ImageData);
	ParseSectionHeaders(pBuffer, &ImageData);

	ParseImportDirectory(pBuffer, &ImageData);
	ParseDebugDirectory(pBuffer, &ImageData);

EXIT:
	// cleanup resources
	if (INVALID_HANDLE_VALUE != hFile)
		CloseHandle(hFile);
	if (NULL != pBuffer)
		delete[] pBuffer;

	std::cout << '\n' << "[+] Parse complete, press ENTER to exit" << std::endl;
	std::cin.get();

	return 0;
}

// simple sanity check - is this a valid PE image?
BOOL SanityCheck(LPBYTE pBuffer)
{
	if (reinterpret_cast<IMAGE_DOS_HEADER*>(pBuffer)->e_magic != IMAGE_DOS_SIGNATURE)
	{
		return FALSE;
	}

	// TODO: other checks here?

	return TRUE;
}

void ParseDOSHeader(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData)
{
	PIMAGE_DOS_HEADER DOSHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(pBuffer);

	printf("\nDOS Header --------------------------------------------------------\n\n");
	printf("	e_magic:  0x%04X\n", DOSHeader->e_magic);
	printf("	e_lfanew: %lu\n", DOSHeader->e_lfanew);

	pImageData->m_NtHeadersOffset = DOSHeader->e_lfanew;
}

void ParseNTHeaders(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData)
{
	LPBYTE NtHeaderBase = pBuffer + pImageData->m_NtHeadersOffset;
	PIMAGE_NT_HEADERS NTHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(NtHeaderBase);

	printf("\nNT Headers --------------------------------------------------------\n\n");
	printf("	Signature: %u -> ", NTHeaders->Signature);
	printf("'%c%c%c%c'\n", (*NtHeaderBase), (*(NtHeaderBase + 1)), (*(NtHeaderBase + 2)), (*(NtHeaderBase + 3)));
}

void ParseFileHeader(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData)
{
	PIMAGE_FILE_HEADER FileHeader = reinterpret_cast<IMAGE_FILE_HEADER*>(pBuffer + pImageData->m_NtHeadersOffset + FILE_HEADER_OFFSET);
	
	printf("\nFile Header -------------------------------------------------------\n\n");
	printf("	Machine:                 0x%04X\n", FileHeader->Machine);
	printf("	Number of Sections:      %u\n", FileHeader->NumberOfSections);
	printf("	Timestamp:               %u\n", FileHeader->TimeDateStamp);
	printf("	Size of Optional Header: %u\n", FileHeader->SizeOfOptionalHeader);
	printf("	Characteristics:\n");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_RELOCS_STRIPPED,         "IMAGE_FILE_RELOCS_STRIPPED");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_EXECUTABLE_IMAGE,        "IMAGE_FILE_EXECUTABLE_IMAGE");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_LINE_NUMS_STRIPPED,      "IMAGE_FILE_LIN_NUMS_STRIPPED");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_LOCAL_SYMS_STRIPPED,     "IMAGE_FILE_LOCAL_SYMS_STRIPPED");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_AGGRESIVE_WS_TRIM,       "IMAGE_FILE_AGGRESIVE_WS_TRIM");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_LARGE_ADDRESS_AWARE,     "IMAGE_FILE_LARGE_ADDRESS_AWARE");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_BYTES_REVERSED_LO,       "IMAGE_FILE_BYTES_REVERSED_LO");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_32BIT_MACHINE,           "IMAGE_FILE_32BIT_MACHINE");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_DEBUG_STRIPPED,          "IMAGE_FILE_DEBUG_STRIPPED");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP, "IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_SYSTEM,                  "IMAGE_FILE_SYSTEM");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_DLL,                     "IMAGE_FILE_DLL");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_UP_SYSTEM_ONLY,          "IMAGE_FILE_UP_SYSTEM_ONLY");
	PrintCharacteristic(FileHeader->Characteristics, IMAGE_FILE_BYTES_REVERSED_HI,       "IMAGE_FILE_BYTES_REVERSED_HI");

	// record data for later use
	pImageData->m_NumberOfSections = FileHeader->NumberOfSections;
	pImageData->m_SizeofOptionalHeader = FileHeader->SizeOfOptionalHeader;
}

// helper functionally to conditionally print characteristic if set
void PrintCharacteristic(DWORD dCharacteristics, DWORD dMask, const char* str)
{
	if ((dCharacteristics & dMask) > 0)
	{
		printf("	- %s\n", str);
	}
}

void ParseOptionalHeader(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData)
{
	PIMAGE_OPTIONAL_HEADER OptionalHeader = reinterpret_cast<IMAGE_OPTIONAL_HEADER*>(pBuffer + pImageData->m_NtHeadersOffset + OPTIONAL_HEADER_OFFSET);
	
	printf("\nOptional Header ---------------------------------------------------\n\n");

	WORD Magic = OptionalHeader->Magic;
	if (IMAGE_NT_OPTIONAL_HDR32_MAGIC == Magic)
	{
		// this is a 32 bit executable
		ParseOptionalHeader32(pBuffer, pImageData);
	}
	else if (IMAGE_NT_OPTIONAL_HDR64_MAGIC == Magic)
	{
		// this is a 64 bit executable
		ParseOptionalHeader64(pBuffer, pImageData);
	}
	else
	{
		// this is a ROM image, or an invalid executable
		// TODO: handle this case?
	}
}

void ParseOptionalHeader32(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData)
{
	PIMAGE_OPTIONAL_HEADER32 OptionalHeader = reinterpret_cast<IMAGE_OPTIONAL_HEADER32*>(pBuffer + pImageData->m_NtHeadersOffset + OPTIONAL_HEADER_OFFSET);

	printf("	Magic:                      0x%04X\n", OptionalHeader->Magic);
	printf("	Address of Entry Point:     %u\n", OptionalHeader->AddressOfEntryPoint);
	printf("	Size of Code:               %u\n", OptionalHeader->SizeOfCode);
	printf("	Size of Initialized Data:   %u\n", OptionalHeader->SizeOfInitializedData);
	printf("	Size of Uninitialized Data: %u\n", OptionalHeader->SizeOfUninitializedData);
}

void ParseOptionalHeader64(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData)
{
	PIMAGE_OPTIONAL_HEADER64 OptionalHeader = reinterpret_cast<IMAGE_OPTIONAL_HEADER64*>(pBuffer + pImageData->m_NtHeadersOffset + OPTIONAL_HEADER_OFFSET);

	printf("	Magic:                     0x%04X\n", OptionalHeader->Magic);
	printf("	Address of Entry Point:     %u\n", OptionalHeader->AddressOfEntryPoint);
	printf("	Size of Code:               %u\n", OptionalHeader->SizeOfCode);
	printf("	Size of Initialized Data:   %u\n", OptionalHeader->SizeOfInitializedData);
	printf("	Size of Uninitialized Data: %u\n", OptionalHeader->SizeOfUninitializedData);
}

void ParseDataDirectories(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData)
{
	PIMAGE_OPTIONAL_HEADER64 OptionalHeader = reinterpret_cast<IMAGE_OPTIONAL_HEADER64*>(pBuffer + pImageData->m_NtHeadersOffset + OPTIONAL_HEADER_OFFSET);

	printf("\nData Directories --------------------------------------------------\n\n");

	for (BYTE i = 0; i < N_DATA_DIRECTORIES; i++)
	{
		auto* pImageDataDir = reinterpret_cast<IMAGE_DATA_DIRECTORY*>(&OptionalHeader->DataDirectory[i]);
		ParseDataDirectory(pImageDataDir, pImageData, i);
	}
}

void ParseDataDirectory(IMAGE_DATA_DIRECTORY* pImageDataDir, PPE_IMAGE_DATA pImageData, BYTE index)
{
	PrintDataDirectoryName(index);
	printf("	Virtual Address: %u\n", pImageDataDir->VirtualAddress);
	printf("	Size:            %u\n", pImageDataDir->Size);

	// record the VA and size for later use
	pImageData->m_DataDirectories[index].m_VirtualAddress = pImageDataDir->VirtualAddress;
	pImageData->m_DataDirectories[index].m_Size = pImageData->m_SizeofOptionalHeader;
}

void PrintDataDirectoryName(BYTE index)
{
	std::string dirName{};

	printf("[0x%02X] ", index);

	switch (index)
	{
	case 0: dirName  = "IMAGE_DIRECTORY_ENTRY_EXPORT"; break;
	case 1: dirName  = "IMAGE_DIRECTORY_ENTRY_IMPORT"; break;
	case 2: dirName  = "IMAGE_DIRECTORY_ENTRY_RESOURCE"; break;
	case 3: dirName  = "IMAGE_DIRECTORY_ENTRY_EXCEPTION"; break;
	case 4: dirName  = "IMAGE_DIRECTORY_ENTRY_SECURITY"; break;
	case 5: dirName  = "IMAGE_DIRECTORY_ENTRY_BASERELOC"; break;
	case 6: dirName  = "IMAGE_DIRECTORY_ENTRY_DEBUG"; break;
	case 7: dirName  = "IMAGE_DIRECTORY_ENTRY_COPYRIGHT"; break;
	case 8: dirName  = "IMAGE_DIRECTORY_ENTRY_GLOBALPTR"; break;
	case 9: dirName  = "IMAGE_DIRECTORY_ENTRY_TLS"; break;
	case 10: dirName = "IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG"; break;
	case 11: dirName = "IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT"; break;
	case 12: dirName = "IMAGE_DIRECTORY_ENTRY_IAT"; break;
	case 13: dirName = "IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT"; break;
	case 14: dirName = "IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR"; break;
	case 15: dirName = "IMAGE_DIRECTORY_ENTRY_RESERVED"; break;
	}

	printf("%s\n", dirName.c_str());
}

void ParseSectionHeaders(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData)
{
	BYTE* pSectionHeadersBase = pBuffer + pImageData->m_NtHeadersOffset + OPTIONAL_HEADER_OFFSET + pImageData->m_SizeofOptionalHeader;

	printf("\nSection Headers ---------------------------------------------------\n");

	for (WORD i = 0; i < pImageData->m_NumberOfSections; i++)
	{
		auto* pSectionHeader = reinterpret_cast<IMAGE_SECTION_HEADER*>(pSectionHeadersBase + i * IMAGE_SIZEOF_SECTION_HEADER);

		printf("\n");
		printf("	Name:                %.*s\n", IMAGE_SIZEOF_SHORT_NAME, pSectionHeader->Name);
		printf("	Virtual Address:     %u\n", pSectionHeader->VirtualAddress);
		printf("	Size of Raw Data     %u\n", pSectionHeader->SizeOfRawData);
		printf("	Pointer to Raw Data: 0x%X\n", pSectionHeader->PointerToRawData);

		// TODO: horribly inefficient and error prone object management, I am truly ashamed...
		std::string str(reinterpret_cast<char*>(pSectionHeader->Name), IMAGE_SIZEOF_SHORT_NAME);
		pImageData->m_SectionHeaders.push_back(
			new SectionHeader(
				str, 
				pSectionHeader->VirtualAddress, 
				pSectionHeader->SizeOfRawData, 
				pSectionHeader->PointerToRawData)
		);
	}
}

void ParseImportDirectory(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData)
{
	DWORD directoryRVA = pImageData->m_DataDirectories[IMAGE_DIRECTORY_ENTRY_IMPORT].m_VirtualAddress;

	ULONG offset = RvaToOffset(directoryRVA, pImageData);
	if (0 == offset)
	{
		printf("Failed to parse Import Directory\n");
		return;
	}

	auto* pImportDescriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(&pBuffer[offset]);

	// TODO: the hard part of parsing imports
}

void ParseDebugDirectory(LPBYTE pBuffer, PPE_IMAGE_DATA pImageData)
{
	DWORD directoryRVA = pImageData->m_DataDirectories[IMAGE_DIRECTORY_ENTRY_DEBUG].m_VirtualAddress;

	ULONG offset = RvaToOffset(directoryRVA, pImageData);
	if (0 == offset)
	{
		printf("Failed to parse Debug Directory\n");
		return;
	}

	auto* pDebugDirectory = reinterpret_cast<IMAGE_DEBUG_DIRECTORY*>(&pBuffer[offset]);

	printf("\nDebug Directory ------------------------------------------------------\n\n");

	printf("	Characteristics:     0x%X\n", pDebugDirectory->Characteristics);
	printf("	TimeDateStamp:       0x%X\n", pDebugDirectory->TimeDateStamp);
	printf("	Major Version:       %u\n", pDebugDirectory->MajorVersion);
	printf("	Minor Version        %u\n", pDebugDirectory->MinorVersion);
	printf("	Type:                0x%X\n", pDebugDirectory->Type);
	printf("	Size of Data:        0x%X\n", pDebugDirectory->SizeOfData);
	printf("	Address of Raw Data: 0x%X\n", pDebugDirectory->AddressOfRawData);
	printf("	Pointer to Raw Data: 0x%X\n", pDebugDirectory->PointerToRawData);
}

ULONG RvaToOffset(DWORD va, PPE_IMAGE_DATA pImageData)
{
	SectionHeader* sectionHeader = nullptr;

	for (SectionHeader* sh : pImageData->m_SectionHeaders)
	{
		if ((va >= sh->m_VirtualAddress) && (va < sh->m_VirtualAddress + sh->m_SizeOfRawData))
		{
			sectionHeader = sh;
			break;
		}
	}

	if (nullptr == sectionHeader)
	{
		return 0;
	}

	return sectionHeader->m_PointerToRawData + (va - sectionHeader->m_VirtualAddress);
}

// helper function to print program usage information
void PrintUsage()
{
	printf("[USAGE]: Parser.exe <filename>\n");
}

// helper function to print error messages
void PrintError(const char *msg)
{
	printf("[ERROR]: %s (0x%08X)\n", msg, GetLastError());
}
