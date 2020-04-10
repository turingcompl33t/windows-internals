// nested_jobs.cpp
//
// Exploration of nested job semantics.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include nested_jobs.cpp

#include <windows.h>

#include <string>
#include <optional>
#include <iostream>

#include <wdl/handle.hpp>
#include <wdl/process/scoped_process.hpp>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

using null_handle = wdl::handle::null_handle;
using scoped_process = wdl::process::scoped_process;

void error(
    std::string const& msg,
    unsigned long code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << " Code: " << std::hex << code << std::dec
        << std::endl;
}

// set the CPU rate control for the specified job to a hard cap
bool set_job_cpu_rate_hard_cap(HANDLE job, unsigned long rate)
{
    auto info = JOBOBJECT_CPU_RATE_CONTROL_INFORMATION{};
    info.ControlFlags = 
        JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP |
        JOB_OBJECT_CPU_RATE_CONTROL_ENABLE;
    
    // CpuRate is expressed as number of cycles per 10,000 cycles
    info.CpuRate = rate*100;

    return ::SetInformationJobObject(
        job,
        JobObjectCpuRateControlInformation,
        &info,
        sizeof(info));
}

std::optional<unsigned long> get_job_cpu_rate_hard_cap(HANDLE job)
{
    auto info   = JOBOBJECT_CPU_RATE_CONTROL_INFORMATION{};
    auto outlen = unsigned long{};

    if (!::QueryInformationJobObject(
        job,
        JobObjectCpuRateControlInformation,
        &info,
        sizeof(info),
        &outlen))
    {
        return std::nullopt;
    }

    return (info.CpuRate / 100);
}

bool create_job_hierarchy_with_cpu_rates(
    unsigned long rate_parent,
    unsigned long rate_child
    )
{
    // create the parent job objects
    auto parent = null_handle{ ::CreateJobObjectW(nullptr, nullptr) };
    auto child  = null_handle{ ::CreateJobObjectW(nullptr, nullptr) };

    if (!parent || !child)
    {
        error("CreateJobObject() failed");
        return false;
    }

    // set the job CPU rates
    if (!set_job_cpu_rate_hard_cap(parent.get(), rate_parent) || 
        !set_job_cpu_rate_hard_cap(child.get(), rate_child))
    {
        error("SetInformationJobObject() failed");
        return false;
    }

    // create the child process
    auto process = scoped_process{"notepad.exe"};
    if (!process)
    {
        error("CreateProcess() failed");
        return false;
    }

    // add the process to the parent job
    if(!::AssignProcessToJobObject(parent.get(), process.get()))
    {
        error("Failed to add process to parent job");
        return false;
    }

    // add the process to the child job 
    if (!::AssignProcessToJobObject(child.get(), process.get()))
    {
        error("Failed to add process to child job");
        return false;
    }

    return true;
}

// attempt to create nested job hierarchy wherein the
// nested job imposes stricter constraints than parent
bool nest_with_tight_constraint()
{   
    // parent has 35% hard cap, child has 25% hard cap
    return create_job_hierarchy_with_cpu_rates(35, 25);
}

// attempt to create a nested job hierarchy wherein the
// nested job imposes looser contraints than parent
bool nest_with_loose_constraint()
{
    // parent has 25% hard cap, child has 35% hard cap
    return create_job_hierarchy_with_cpu_rates(25, 35);
}

int main()
{
    auto res1 = nest_with_tight_constraint();
    auto res2 = nest_with_loose_constraint();

    std::cout << std::boolalpha;
    std::cout << "Create nested job with tighter constraint: " << res1 << '\n';
    std::cout << "Create nested job with looser constraint:  " << res2 << '\n';

    return STATUS_SUCCESS_I;
}