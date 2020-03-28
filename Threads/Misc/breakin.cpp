// breakin.cpp
//
// Forcing a debug break in a specified process with CreateRemoteThread()
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I C:\Dev\WDL/include breakin.cpp

#include <windows.h>
#include <wdl/handle.hpp>

#include <string>
#include <iostream>
#include <stdexcept>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

int main(int argc, char* argv[])
{
    using wdl::handle::null_handle;

    if (argc != 2)
    {
        std::cout << "[-] Invalid arguments\n";
        std::cout << "[-] Usage: breakin <PID>\n";
        return STATUS_FAILURE_I;
    }

    auto pid = unsigned long{};

    try
    {
        pid = std::stoul(argv[1]);
    }
    catch (std::exception const& e)
    {
        std::cout << "[-] Failed to parse PID: \n";
        std::cout << e.what() << '\n';
        return STATUS_FAILURE_I;
    }

    auto process = null_handle
    {
        ::OpenProcess(
            PROCESS_CREATE_THREAD |
            PROCESS_QUERY_INFORMATION |
            PROCESS_VM_OPERATION |
            PROCESS_VM_READ |
            PROCESS_VM_WRITE,
            FALSE, 
            pid
        )
    };

    if (!process)
    {
        std::cout << "[-] Failed to open target process\n";
        return STATUS_FAILURE_I;
    }

    auto* thread_entry = ::GetProcAddress(
        ::GetModuleHandleW(L"kernel32"), 
        "DebugBreak");

    auto thread = null_handle
    {
        ::CreateRemoteThread(
            process.get(),
            nullptr,
            0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(
                thread_entry),
            nullptr,
            0, 
            nullptr
        )
    };

    if (!thread)
    {
        std::cout << "[-] Failed to create remote thread\n";
        return STATUS_FAILURE_I;
    }

    std::cout << "[+] Successfully started remote thread\n";

    // don't wait for thread to complete

    return STATUS_SUCCESS_I;
}