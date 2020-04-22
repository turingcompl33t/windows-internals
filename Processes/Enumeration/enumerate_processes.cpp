// enumerate_process.cpp
//
// Demonstration of various methods for enumerating processes.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 enumerate_processes.cpp

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

void enumerate_win32();
void enumerate_toolhelp();
void enumerate_wts();

int parse_method_id(char* argv[]);

void log_info(std::string_view msg);
void log_warning(std::string_view msg);
void log_error(std::string_view msg);

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        log_warning("Invalid arguments");
        log_warning("Usage: EnumerateProcesses <METHOD ID>");
        std::cout << "\t0 -> EnumerateProcesses()" << '\n'
                  << "\t1 -> ToolHelp32" << '\n'
                  << "\t2 -> WTSEnumerateProcesses()" << '\n'
                  << std::endl;
        return STATUS_FAILURE_I;
    }

    auto method_id = parse_method_id(argv);
    if (method_id < 0)
    {
        return STATUS_FAILURE_I;
    }

    switch (method_id)
    {
        case 0:
        {
            enumerate_win32();
            break;
        }
        case 1:
        {
            enumerate_toolhelp();
            break;
        }
        case 2:
        {
            enumerate_wts();
            break;
        }
        default:
        {
            log_warning("Invalid method ID specified");
            return STATUS_FAILURE_I;
        }
    }

    return STATUS_SUCCESS_I;
}



// use the win32 API to enumerate processes
void enumerate_win32()
{
    auto actual_size = unsigned long{};
    auto max_count   = unsigned long{256};
    auto count       = unsigned long{};

    std::unique_ptr<unsigned long[]> pids;

    log_info("Enumerating processes with Win32 EnumProcesses()");

    // loop until we have enough space for all PIDs
    for (;;)
    {   
        pids = std::make_unique<unsigned long[]>(max_count);
        if (!::EnumProcesses(pids.get(), 
                max_count*sizeof(unsigned long), 
                &actual_size))
        {
            log_error("Failed to enumerate processes (EnumProcesses())");
            return;
        }

        count = actual_size / sizeof(unsigned long);
        if (count < max_count)
        {
            break;
        }

        // resize and try again
        max_count *= 2;
    }

    printf("[+] Enumerated %u processes:\n", count);

    for (auto i = 0u; i < count; ++i)
    {
        auto pid = pids[i];
        HANDLE hProcess = ::OpenProcess(
            PROCESS_QUERY_LIMITED_INFORMATION, 
            FALSE, 
            pid);

        if (NULL == hProcess)
        {
            printf("Failed to acquire handle to process %5u (GLE: %4u)\n", 
                pid, 
                ::GetLastError());
            continue;
        }

        auto start_time = FILETIME{ 0 };
        auto dummy_time = FILETIME{};
        auto system_time = SYSTEMTIME{};

        ::GetProcessTimes(hProcess, &start_time, &dummy_time, &dummy_time, &dummy_time);     
        ::FileTimeToLocalFileTime(&start_time, &start_time);
        ::FileTimeToSystemTime(&start_time, &system_time);

        BOOL GotName;
        WCHAR ExeName[MAX_PATH];
        DWORD NameLength = MAX_PATH;

        GotName = ::QueryFullProcessImageNameW(hProcess, 0, ExeName, &NameLength);

        printf("[%5u]: %ws\n", pid, GotName ? ExeName : L"Name Unknown");
        printf("\tStart Time: %d/%d/%d %02d:%02d:%02d\n", 
            system_time.wDay, 
            system_time.wMonth, 
            system_time.wYear, 
            system_time.wHour, 
            system_time.wMinute, 
            system_time.wSecond
            );

        ::CloseHandle(hProcess);
    }
}

// enumerate processes with the Toolhelp32 API
void enumerate_toolhelp()
{
    log_info("Enumerating processes with Toolhelp32");

    auto hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot)
    {
        log_error("Failed to create process snapshot (CreateToolhelp32Snapshot())");
        return;
    }

    auto process_entry = PROCESSENTRY32W{};
    process_entry.dwSize = sizeof(PROCESSENTRY32W);

    if (!::Process32FirstW(hSnapshot, &process_entry))
    {
        log_error("Failed to enumerate processes (Process32First())");
        CloseHandle(hSnapshot);
        return;
    }

    do 
    {
        printf("[%5u]: %ws\n", process_entry.th32ProcessID, process_entry.szExeFile);
    } while (::Process32NextW(hSnapshot, &process_entry));

    ::CloseHandle(hSnapshot);
}

// enumerate processes with the Windows Terminal Services API
void enumerate_wts()
{
    log_info("Enumerating processes with WTSEnumerateProcesses()");

    auto count = unsigned long{};
    auto process_info = PWTS_PROCESS_INFOW{};

    if (!::WTSEnumerateProcessesW(
        WTS_CURRENT_SERVER_HANDLE, 
        0, 1, 
        &process_info, 
        &count))
    {
        log_error("Failed to enumerate processes (WTSEnumerateProcesses())");
        return;
    }

    printf("[+] Enumerated %u processes:\n", count);

    for (auto i = 0u; i < count; ++i)
    {
        auto info = process_info + i;

        // TODO: convert user SID to string and display
        printf("[%5u]: %ws\n", info->ProcessId, info->pProcessName);
        printf("\tSession ID: %u\n", info->SessionId);
    }    
    
    // required cleanup
    ::WTSFreeMemory(process_info);
}

int parse_method_id(char* argv[])
{
    int method_id = -1;

    try
    {
        method_id = std::stoi(argv[1]);
    }
    catch (std::out_of_range)
    {
        log_warning("Failed to parse Method ID (out of range)");
    }
    catch (std::invalid_argument)
    {
        log_warning("Failed to parse Method ID (conversion cannot be performed)");
    }

    return method_id;
}

void log_info(std::string_view msg)
{
    std::cout << "[+] " << msg << std::endl;
}

void log_warning(std::string_view msg)
{
    std::cout << "[-] " << msg << std::endl;
}

void log_error(std::string_view msg)
{
    std::cout << "[!] " << msg << '\n';
    std::cout << "[!]\tGLE: " << GetLastError() << '\n';
    std::cout << std::endl;
}