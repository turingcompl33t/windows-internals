// Win32Walk.cpp
// Perform directory walk using the Win32 API.

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tchar.h>

#include <cstdio>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

enum class FileTypeEnum
{
    TYPE_FILE,
    TYPE_DIRECTORY,
    TYPE_DOT
};

VOID TraverseDirectory(LPCTSTR pathName);
FileTypeEnum FileType(LPWIN32_FIND_DATA pFileData);

INT _tmain(INT argc, PTCHAR argv[])
{
    if (argc != 2)
    {
        _tprintf(_T("[-] Invalid arguments\n"));
        _tprintf(_T("[-] Usage: %s <DIRWALK ROOT>\n"), argv[0]);
        return STATUS_FAILURE_I;
    }

    TCHAR currentPath[MAX_PATH + 1];
    GetCurrentDirectory(MAX_PATH, currentPath);
    SetCurrentDirectory(argv[1]);
    
    TraverseDirectory(_T("*"));

    SetCurrentDirectory(currentPath);

    return STATUS_SUCCESS_I;
}

VOID TraverseDirectory(LPCTSTR pathName)
{
    HANDLE searchHandle;
    WIN32_FIND_DATA findData;

    DWORD iPass;    
    FileTypeEnum fType;
    TCHAR currentPath[MAX_PATH + 1];

    GetCurrentDirectory(MAX_PATH, currentPath);

    for (iPass = 1; iPass <= 2; iPass++)
    {
        // Pass 1 -> list files
        // Pass 2 -> recurse into directories

        searchHandle = FindFirstFile(pathName, &findData);
        do
        {
            fType = FileType(&findData);
            if (iPass == 1)
            {
                // process individual file
                _tprintf(_T("%s\n"), findData.cFileName);
            }

            if (fType == FileTypeEnum::TYPE_DIRECTORY && iPass == 2)
            {
                // process a subdirectory
                _tprintf(_T("\n%s\\%s:\n"), currentPath, findData.cFileName);

                SetCurrentDirectory(findData.cFileName);
                TraverseDirectory(_T("*"));
                SetCurrentDirectory(_T(".."));
            }
        } while (FindNextFile(searchHandle, &findData));
        FindClose(searchHandle);
    }
}

FileTypeEnum FileType(LPWIN32_FIND_DATA pFileData)
{
    BOOL isDir;
    FileTypeEnum fType;
    fType = FileTypeEnum::TYPE_FILE;
    isDir = (pFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    if (isDir)
    {
        if (lstrcmp(pFileData->cFileName, _T(".")) == 0
            || lstrcmp(pFileData->cFileName, _T("..")) == 0)
        {
            fType = FileTypeEnum::TYPE_DOT;
        }
        else
        {
            fType = FileTypeEnum::TYPE_DIRECTORY;
        }
    }

    return fType;
}