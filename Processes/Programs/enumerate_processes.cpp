// enumerate_process.cpp
// Demonstration of various methods for enumerating processes.
//
// BUILD:
//  cl /EHsc /nologo /std:c++17 enumerate_processes.cpp

#include <windows.h>
#include <Psapi.h>
#include <WtsApi32.h>
#include <TlHelp32.h>

#include <string>
#include <cstdio>
#include <memory>
#include <iostream>
#include <string_view>

#pragma comment(lib, "wtsapi32.lib")

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

void EnumerateProcessesWin32();
void EnumerateProcessesToolhelp32();
void EnumerateProcessesWTS();

int ParseMethodId(char* argv[]);

void LogInfo(std::string_view msg);
void LogWarning(std::string_view msg);
void LogError(std::string_view msg);

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        LogWarning("Invalid arguments");
        LogWarning("Usage: EnumerateProcesses <METHOD ID>");
        std::cout << "\t0 -> EnumerateProcesses()" << '\n';
        std::cout << "\t1 -> ToolHelp32" << '\n';
        std::cout << "\t2 -> WTSEnumerateProcesses()" << '\n';
        std::cout << std::endl;
        return STATUS_FAILURE_I;
    }

    auto MethodId = ParseMethodId(argv);
    if (MethodId < 0)
    {
        return STATUS_FAILURE_I;
    }

    switch (MethodId)
    {
        case 0:
        {
            EnumerateProcessesWin32();
            break;
        }
        case 1:
        {
            EnumerateProcessesToolhelp32();
            break;
        }
        case 2:
        {
            EnumerateProcessesWTS();
            break;
        }
        default:
        {
            LogWarning("Invalid method ID specified");
            return STATUS_FAILURE_I;
        }
    }

    return STATUS_SUCCESS_I;
}



// use the win32 API to enumerate processes
void EnumerateProcessesWin32()
{
    DWORD dwActualSize;
    DWORD dwMaxCount = 256;
    DWORD dwCount = 0;

    std::unique_ptr<DWORD[]> Pids;

    LogInfo("Enumerating processes with Win32 EnumProcesses()");

    // loop until we have enough space for all PIDs
    for (;;)
    {   
        Pids = std::make_unique<DWORD[]>(dwMaxCount);
        if (!::EnumProcesses(Pids.get(), dwMaxCount*sizeof(DWORD), &dwActualSize))
        {
            LogError("Failed to enumerate processes (EnumProcesses())");
            return;
        }

        dwCount = dwActualSize / sizeof(DWORD);
        if (dwCount < dwMaxCount)
        {
            break;
        }

        // resize and try again
        dwMaxCount *= 2;
    }

    printf("[+] Enumerated %u processes:\n", dwCount);

    for (DWORD i = 0; i < dwCount; ++i)
    {
        DWORD pid = Pids[i];
        HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (NULL == hProcess)
        {
            printf("Failed to acquire handle to process %5u (GLE: %4u)\n", pid, GetLastError());
            continue;
        }


        FILETIME StartTime = { 0 };
        FILETIME DummyTime;
        SYSTEMTIME SystemTime;

        ::GetProcessTimes(hProcess, &StartTime, &DummyTime, &DummyTime, &DummyTime);     
        ::FileTimeToLocalFileTime(&StartTime, &StartTime);
        ::FileTimeToSystemTime(&StartTime, &SystemTime);

        BOOL GotName;
        WCHAR ExeName[MAX_PATH];
        DWORD NameLength = MAX_PATH;

        GotName = ::QueryFullProcessImageNameW(hProcess, 0, ExeName, &NameLength);

        printf("[%5u]: %ws\n", pid, GotName ? ExeName : L"Name Unknown");
        printf("\tStart Time: %d/%d/%d %02d:%02d:%02d\n", 
            SystemTime.wDay, 
            SystemTime.wMonth, 
            SystemTime.wYear, 
            SystemTime.wHour, 
            SystemTime.wMinute, 
            SystemTime.wSecond
        );

        ::CloseHandle(hProcess);
    }
}

// enumerate processes with the Toolhelp32 API
void EnumerateProcessesToolhelp32()
{
    LogInfo("Enumerating processes with Toolhelp32");

    HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot)
    {
        LogError("Failed to create process snapshot (CreateToolhelp32Snapshot())");
        return;
    }

    PROCESSENTRY32W ProcessEntry;
    ProcessEntry.dwSize = sizeof(PROCESSENTRY32W);

    if (!::Process32FirstW(hSnapshot, &ProcessEntry))
    {
        LogError("Failed to enumerate processes (Process32First())");
        CloseHandle(hSnapshot);
        return;
    }

    do 
    {
        printf("[%5u]: %ws\n", ProcessEntry.th32ProcessID, ProcessEntry.szExeFile);
    } while (::Process32NextW(hSnapshot, &ProcessEntry));

    ::CloseHandle(hSnapshot);
}

// enumerate processes with the Windows Terminal Services API
void EnumerateProcessesWTS()
{
    LogInfo("Enumerating processes with WTSEnumerateProcesses()");

    DWORD Count;
    PWTS_PROCESS_INFOW pProcessInfo;

    if (!::WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &Count))
    {
        LogError("Failed to enumerate processes (WTSEnumerateProcesses())");
        return;
    }

    printf("[+] Enumerated %u processes:\n", Count);

    for (DWORD i = 0; i < Count; ++i)
    {
        auto pInfo = pProcessInfo + i;

        // TODO: convert user SID to string and display
        printf("[%5u]: %ws\n", pInfo->ProcessId, pInfo->pProcessName);
        printf("\tSession ID: %u\n", pInfo->SessionId);
    }    
    
    // required cleanup
    ::WTSFreeMemory(pProcessInfo);
}

int ParseMethodId(char* argv[])
{
    int MethodId = -1;

    try
    {
        MethodId = std::stoi(argv[1]);
    }
    catch (std::out_of_range)
    {
        LogWarning("Failed to parse Method ID (out of range)");
    }
    catch (std::invalid_argument)
    {
        LogWarning("Failed to parse Method ID (conversion cannot be performed)");
    }

    return MethodId;
}

void LogInfo(std::string_view msg)
{
    std::cout << "[+] " << msg << std::endl;
}

void LogWarning(std::string_view msg)
{
    std::cout << "[-] " << msg << std::endl;
}

void LogError(std::string_view msg)
{
    std::cout << "[!] " << msg << '\n';
    std::cout << "[!]\tGLE: " << GetLastError() << '\n';
    std::cout << std::endl;
}