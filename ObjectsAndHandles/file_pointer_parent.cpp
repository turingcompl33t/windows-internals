// file_pointer_parent.cpp
//
// Interaction of handling inheritance and file pointer.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include file_pointer_parent.cpp

#include <windows.h>
#include <string.h>

#include <string>
#include <optional>
#include <iostream>

#include <wdl/debug.hpp>
#include <wdl/handle.hpp>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

using null_handle    = wdl::handle::null_handle;
using invalid_handle = wdl::handle::invalid_handle;

void error(
    std::string const& msg,
    unsigned long code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << "Code: " << std::hex << code << std::dec
        << std::endl;
}

invalid_handle scoped_file(std::wstring const& path)
{
    auto sa = SECURITY_ATTRIBUTES{};
    sa.bInheritHandle = TRUE;

    return invalid_handle
    {
        ::CreateFileW(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            &sa,
            CREATE_NEW,
            FILE_FLAG_DELETE_ON_CLOSE,
            nullptr)
    };
}

std::optional<unsigned long long> get_file_pointer(HANDLE file)
{
    auto src = LARGE_INTEGER{};
    auto dst = LARGE_INTEGER{};

    src.QuadPart = 0;

    auto res = ::SetFilePointerEx(file, src, &dst, FILE_CURRENT);
    if (!res)
    {
        return std::nullopt;
    }
    return dst.QuadPart;
}

bool set_file_pointer(HANDLE file, unsigned long long offset)
{
    auto src = LARGE_INTEGER{};
    auto dst = LARGE_INTEGER{};

    src.QuadPart = offset;

    return ::SetFilePointerEx(file, src, &dst, FILE_BEGIN);
}

int main()
{
    // create inheritable, scoped file
    auto file = scoped_file(L"test.txt");
    if (!file)
    {
        error("CreateFile() failed");
        return STATUS_FAILURE_I;
    }

    // query the initial file pointer
    auto offset = get_file_pointer(file.get());
    if (!offset.has_value())
    {
        error("GetFilePointer() failed");
        return STATUS_FAILURE_I;
    }

    std::cout << "[PARENT] Offset = " << offset.value() << '\n';
    
    // create a child process the inherits the file handle

    wchar_t buffer[MAX_PATH];
    swprintf_s(buffer, L"file_pointer_child.exe %lu", ::HandleToULong(file.get()));

    auto process_info = PROCESS_INFORMATION{};
    auto startup_info = STARTUPINFOW{};

    if (!::CreateProcessW(
        nullptr,
        buffer,
        nullptr,
        nullptr,
        TRUE,
        0,
        nullptr,
        nullptr,
        &startup_info,
        &process_info))
    {
        error("CreateProcess() failed");
        return STATUS_FAILURE_I;
    }

    ::CloseHandle(process_info.hThread);
    auto child = null_handle{ process_info.hProcess };

    // wait for child to exit
    ::WaitForSingleObject(child.get(), INFINITE);

    offset = get_file_pointer(file.get());
    if (!offset.has_value())
    {
        error("GetFilePointer() failed");
        return STATUS_FAILURE_I;
    }

    std::cout << "[PARENT] Offset = " << offset.value() << '\n';

    return STATUS_SUCCESS_I;
}
