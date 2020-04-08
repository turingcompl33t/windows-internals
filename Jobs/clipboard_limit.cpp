// clipboard_limit.cpp
//
// Limit process access to the clipboard with job restrictions.
//
// Usage
//  clipboard_limit.exe <COMMAND> <PID> [<PID> ...]
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 clipboard_limit.cpp

#include <windows.h>
#include <string.h>

#include <string>
#include <vector>
#include <optional>
#include <iostream>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

enum class limit_type
{
    invalid,
    read,
    write,
    read_write
};

void error(
    std::string const& msg,
    unsigned long code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << "Code: " << std::hex << code << std::dec
        << std::endl;
}

[[nodiscard]]
std::optional<limit_type> parse_limit_type(char* arr[])
{
    if (_stricmp(arr[0], "r"))
    {
        return limit_type::read;
    }
    else if (_stricmp(arr[0], "w")) 
    {
        return limit_type::write;
    }
    else if (_stricmp(arr[0], "rw"))
    {
        return limit_type::read_write;
    }
    else
    {
        return std::nullopt;
    }
}

[[nodiscard]]
std::optional<std::vector<unsigned long>>
parse_pids(char* arr[], int count)
{
    auto pids = std::vector<unsigned long>{};

    for (auto i = 0; i < count; ++i)
    {
        try
        {
            pids.push_back(std::stoul(arr[i]));
        }
        catch (std::exception const&)
        {
            // std::invalid_argument or std::out_of_range
            return std::nullopt;
        }
    }

    return pids;
}

bool set_job_clipboard_limits(HANDLE job, limit_type limit)
{   
    auto info = JOBOBJECT_BASIC_UI_RESTRICTIONS{};

    if (limit == limit_type::read) 
        info.UIRestrictionsClass = JOB_OBJECT_UILIMIT_READCLIPBOARD;
    else if (limit == limit_type::write) 
        info.UIRestrictionsClass = JOB_OBJECT_UILIMIT_WRITECLIPBOARD;
    else 
        info.UIRestrictionsClass = JOB_OBJECT_UILIMIT_READCLIPBOARD | JOB_OBJECT_UILIMIT_WRITECLIPBOARD;

    return ::SetInformationJobObject(
        job, 
        JobObjectBasicUIRestrictions,
        &info,
        sizeof(info));
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
        std::cout << "Invalid arguments\n";
        std::cout << "Usage: " << argv[0] << "<COMMAND> <PID> [<PID> ...]\n";
        return STATUS_FAILURE_I;
    }

    auto limit_opt = parse_limit_type(argv+1);
    auto pids_opt = parse_pids(argv+2, argc-2);
    if (!limit_opt || !pids_opt)
    {
        std::cout << "Invalid arguments; failed to parse\n";
        return STATUS_FAILURE_I;
    }

    auto& limit = limit_opt.value();
    auto& pids  = pids_opt.value();

    auto job = ::CreateJobObjectW(nullptr, nullptr);
    if (NULL == job)
    {
        error("CreateJobObject() failed");
        return STATUS_FAILURE_I;
    }

    if (!set_job_clipboard_limits(job, limit))
    {
        error("SetInformationJobObject() failed");
        ::CloseHandle(job);
        return STATUS_FAILURE_I;
    }

    for (auto pid : pids)
    {
        if (add_process_to_job(job, pid))
        {
            std::cout << "Successfully added process " << pid << '\n';
        }
        else
        {
            std::cout << "Failed to add process " << pid << '\n';
        }
    }

    return STATUS_SUCCESS_I;
}