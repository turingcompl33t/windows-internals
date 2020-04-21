// client.cpp
//
// Client application demonstrating asynchronous thread cancellation.

#include <windows.h>
#include <winioctl.h>

#include <iostream>
#include <string_view>

#include <wdl/handle/generic.hpp>

using null_handle = wdl::handle::null_handle;
using invalid_handle = wdl::handle::invalid_handle;

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto THREAD_EXIT_CODE = 1337;

constexpr const auto FILE_DEVICE_CANCELDRV = 0x00008005;
constexpr const auto IOCTL_CANCELDRV_SET_ALERTABLE 
    = CTL_CODE(FILE_DEVICE_CANCELDRV, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS);

void error(
    std::string_view msg,
    unsigned long const code = ::GetLastError()
)
{
    std::cout << "[-] " << msg 
        << " Code: " << std::hex << code << std::dec
        << std::endl;
}

unsigned long QueueUserAPCEx(
    PAPCFUNC      callback, 
    HANDLE        thread, 
    ULONG_PTR     context,
    HANDLE        device_handle
    )
{
    // check for trivial case
    if (thread == ::GetCurrentThread())
    {
        if (!::QueueUserAPC(callback, thread, context))
        {
            return 0;
        }

        ::SleepEx(0, TRUE);
        return 1;
    }

    if (INVALID_HANDLE_VALUE == device_handle)
    {
        // need a valid handle to device!!
        return 0;
    }

    if (::SuspendThread(thread) == -1)
    {
        return 0;
    }

    if (!::QueueUserAPC(callback, thread, context))
    {
        return 0;
    }

    auto bytes_out = unsigned long{};
    if (::DeviceIoControl(
        device_handle, 
        (DWORD)IOCTL_CANCELDRV_SET_ALERTABLE, 
        &thread, 
        sizeof(HANDLE),
        NULL, 0, 
        &bytes_out, 
        0))
    {
    }
    else
    {
        return 0;
    }

    ::ResumeThread(thread);

    return 1;
}

void __stdcall apc_routine(ULONG_PTR)
{
    std::cout << "Goodbye cruel world...\n";
    ::ExitThread(THREAD_EXIT_CODE);
}

unsigned long __stdcall worker(void*)
{
    for (;;)
    {
        std::cout << "Doing some work...\n";
        ::Sleep(500);
    }
}

int main()
{
    // acquire a handle to the support driver (via device symlink)
    auto device = invalid_handle
    {
        ::CreateFileW(
            L"\\\\.\\Global\\CANCELDRV",
            GENERIC_READ | GENERIC_WRITE,
            0, NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL)
    };
    
    if (!device)
    {
        error("Failed to acquire handle to support driver device");
        return STATUS_FAILURE_I;
    }

    std::cout << "[+] Successfully acquired handle to device\n"
        << "[+] Starting thread...\n";

    // spin up a thread
    auto thread = null_handle
    {
        ::CreateThread(nullptr, 0, worker, nullptr, 0, nullptr)
    };

    if (!thread)
    {
        error("CreateThread() failed");
        return STATUS_FAILURE_I;
    }

    // wait a few seconds...
    ::Sleep(3000);

    // cancel the thread with APC
    QueueUserAPCEx(apc_routine, thread.get(), 0, device.get());
    ::WaitForSingleObject(thread.get(), INFINITE);

    auto code = unsigned long{};
    ::GetExitCodeThread(thread.get(), &code);

    std::cout << "[+] Thread successfully cancelled; exit code: " << code << '\n';

    return STATUS_SUCCESS_I;
}
