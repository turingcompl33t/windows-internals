// process_memory_limit.cpp
//
// Per-process memory commit limitation with jobs.
//
// Usage
//  process_memory_limit.exe <COMMIT_LIMIT> <PID> [<PID>...]
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 process_memory_limit.cpp

#include <windows.h>

#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

void error(
    std::string const& msg,
    unsigned long code = ::GetLastError())
{
    std::cout << "[-] " << msg
        << " Code: " << code
        << '\n';
}

[[nodiscard]]
std::vector<unsigned long> parse_pids(char* arr[], int count)
{
    auto pids = std::vector<unsigned long>{};
    for (auto i = 0; i < count; ++i)
    {
        // may throw
        pids.push_back(std::stoul(arr[i]));
    }

    return pids;
}

[[nodiscard]]
unsigned long parse_commit_limit(char* arr[])
{
    // may throw
    return std::stoul(arr[0]);
}

[[nodiscard]]
unsigned long get_page_size()
{
    auto info = SYSTEM_INFO{};
    ::GetSystemInfo(&info);
    return info.dwPageSize;
}

bool set_job_process_memory_limit(HANDLE job, SIZE_T limit_bytes)
{
    auto info = JOBOBJECT_EXTENDED_LIMIT_INFORMATION{};
    info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
    info.ProcessMemoryLimit = limit_bytes;

    return ::SetInformationJobObject(
        job, 
        JobObjectExtendedLimitInformation, 
        &info, 
        sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
}

bool add_process_to_job(HANDLE job, unsigned long pid)
{
    auto process = ::OpenProcess(
        PROCESS_SET_QUOTA | 
        PROCESS_TERMINATE, 
        FALSE, 
        pid);

    if (NULL == process)
    {
        // could not acquire handle to process
        return false;
    }

    auto success = ::AssignProcessToJobObject(job, process);
    ::CloseHandle(process);

    return success;
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "[-] Invalid arguments\n";
        std::cout << "[-] Usage: " << argv[0]
             << " <COMMIT_LIMIT> <PID> [<PID> ...]\n";
        return STATUS_FAILURE_I;
    }

    unsigned long limit_pages{};
    std::vector<unsigned long> pids{};

    try
    {
        limit_pages = parse_commit_limit(argv+1);
        pids = parse_pids(argv+2, argc-2);
    }
    catch (std::exception const&)
    {
        // std::invalid_argument or std::out_of_range
        std::cout << "Invalid aguments; failed to parse\n";
        return STATUS_FAILURE_I;
    }

    auto limit_bytes = limit_pages * get_page_size();

    std::cout << "Setting commit limit of " 
        << limit_pages << " pages"
        << " (" << limit_bytes << " bytes) " 
        << "on processes: ";
    for (auto p : pids) { std::cout << p << " "; }
    std::cout << std::endl;

    auto job = ::CreateJobObjectW(nullptr, nullptr);
    if (NULL == job)
    {
        error("CreateJobObject() failed");
        return STATUS_FAILURE_I;
    }

    if (!set_job_process_memory_limit(job, limit_bytes))
    {
        error("SetInformationJobObject() failed");
        ::CloseHandle(job);
        return STATUS_FAILURE_I;
    }

    for (auto pid : pids)
    {
        if (add_process_to_job(job, pid))
        {
            std::cout << "Successfully added process with PID " << pid << '\n';
        }
        else
        {
            std::cout << "Failed to add process with PID " << pid << '\n';
        }
    }

    // wait for input
    std::cin.get();

    ::CloseHandle(job);
    return STATUS_SUCCESS_I;
}