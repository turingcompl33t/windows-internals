// FormatMessage.cpp
// Demo using FormatMessage() to format system error codes.

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <string>

VOID ReportError(LPCTSTR msg, DWORD dwErrorCode);

INT _tmain(INT argc, PTCHAR argv[])
{
    if (argc != 2)
    {
        _tprintf(_T("Error: invalid arguments\n"));
        _tprintf(_T("Usage: %s <PID>\n"), argv[0]);
        return 1;
    }

    DWORD dwTargetPid;
    HANDLE hTargetProcess;

    try
    {
        dwTargetPid = std::stoul(argv[1]);
    }
    catch (...)
    {
        // lazy error handling
        _tprintf(_T("Error: failed to parse target process PID\n"));
        return 1;
    }

    // naturally, this call will fail with many processes
    // e.g. try against lsass.exe, MsMpEng.exe, etc.
    hTargetProcess = OpenProcess(
        PROCESS_ALL_ACCESS,
        FALSE,
        dwTargetPid
    );

    if (NULL == hTargetProcess)
    {
        // report error without crashing
        ReportError(_T("Failed to acquire handle to target process"), 0);
    }
    else
    {
        _tprintf(_T("Successfully acquired handle to process\n"));
        CloseHandle(hTargetProcess);
    }

    return 0;
}

VOID ReportError(LPCTSTR msg, DWORD dwErrorCode)
{
    DWORD eMsgLen;
    DWORD errNum = GetLastError();
    LPTSTR lpvSysMsg;

    // print user message
    _ftprintf(stderr, _T("%s\n"), msg);
    eMsgLen = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
        | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errNum,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPTSTR>(&lpvSysMsg),
        0,
        NULL
    );
    if (eMsgLen > 0)
    {
        _ftprintf(stderr, _T("%s\n"), lpvSysMsg);
    }
    else
    {
        _ftprintf(stderr, _T("Last Error Number: %d\n"), errNum);
    }

    if (NULL != lpvSysMsg)
    {
        LocalFree(lpvSysMsg);
    }

    // die 
    if (dwErrorCode > 0)
    {
        ExitProcess(dwErrorCode);
    }

    return;
}