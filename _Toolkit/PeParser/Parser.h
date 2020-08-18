/*
 * Parser.h
 * Simple PE32 / PE32+ binary format parser.
 */

#ifndef __PARSER_H
#define __PARSER_H

#include <vector>
#include <Windows.h>

#define FILE_HEADER_OFFSET     (4)
#define OPTIONAL_HEADER_OFFSET (4 + IMAGE_SIZEOF_FILE_HEADER)
#define N_DATA_DIRECTORIES     (16)

typedef struct _DATA_DIRECTORY {
	DWORD m_VirtualAddress;
	DWORD m_Size;
} DATA_DIRECTORY, PDATA_DIRECTORY;

class SectionHeader
{
public:
	std::string m_Name;
	ULONG m_VirtualAddress;
	ULONG m_SizeOfRawData;
	ULONG m_PointerToRawData;

	SectionHeader(std::string &name, ULONG va, ULONG srd, ULONG prd)
		: m_Name(name), m_VirtualAddress(va), m_SizeOfRawData(srd), m_PointerToRawData(prd)
	{
	}

	~SectionHeader() = default;
};

typedef struct _PE_IMAGE_DATA {
	LONG m_NtHeadersOffset;       // file offset to start of NT Headers
	WORD m_NumberOfSections;      // number of sections defined in image
	WORD m_SizeofOptionalHeader;  // size of optional header in bytes
	
	// static array of data directories
	DATA_DIRECTORY m_DataDirectories[N_DATA_DIRECTORIES];

	// dynamic vector of section headers
	std::vector<SectionHeader*> m_SectionHeaders;
} PE_IMAGE_DATA, *PPE_IMAGE_DATA;

#endif // __PARSER_H
