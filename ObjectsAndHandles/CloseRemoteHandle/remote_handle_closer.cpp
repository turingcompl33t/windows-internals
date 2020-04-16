// remote_handle_closer.cpp
//
// Target application for remote handle closure.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include remote_handle_closer.cpp

#include <windows.h>
#include <TlHelp32.h>

#include <wdl/handle/generic.hpp>

#include <string.h>
#include <memory>
#include <optional>
#include <iostream>
#include <string_view>

using null_handle = wdl::handle::null_handle;
using invalid_handle = wdl::handle::invalid_handle;

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto IMAGE_BUFLEN       = 256;
constexpr const auto OBJECT_NAME_BUFLEN = 256;

constexpr const auto STATUS_INFO_LENGTH_MISMATCH = 0xC0000004L;

#define NT_SUCCESS(status) (status >= 0)

enum PROCESSINFOCLASS 
{
    ProcessHandleInformation = 51
};

typedef struct _PROCESS_HANDLE_TABLE_ENTRY_INFO 
{
    HANDLE    HandleValue;
    ULONG_PTR HandleCount;
    ULONG_PTR PointerCount;
    ULONG     GrantedAccess;
    ULONG     ObjectTypeIndex;
    ULONG     HandleAttributes;
    ULONG     Reserved;
} PROCESS_HANDLE_TABLE_ENTRY_INFO, *PPROCESS_HANDLE_TABLE_ENTRY_INFO;

typedef struct _PROCESS_HANDLE_SNAPSHOT_INFORMATION 
{
    ULONG_PTR                       NumberOfHandles;
    ULONG_PTR                       Reserved;
    PROCESS_HANDLE_TABLE_ENTRY_INFO Handles[1];
} PROCESS_HANDLE_SNAPSHOT_INFORMATION, *PPROCESS_HANDLE_SNAPSHOT_INFORMATION;

enum OBJECT_INFORMATION_CLASS 
{
    ObjectNameInformation = 1
};

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;
 
typedef struct _OBJECT_NAME_INFORMATION {
    UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

using nt_query_process_f = NTSTATUS (__stdcall*)(
    HANDLE           ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID            ProcessInformation,
    ULONG            ProcessInformationLength,
    PULONG           ReturnLength
    );

using nt_query_object_f = NTSTATUS (__stdcall*)(
    HANDLE Handle,
    OBJECT_INFORMATION_CLASS ObjectInformationClass,
    PVOID ObjectInformation,
    ULONG ObjectInformationLength,
    PULONG ReturnLength
    );

void error(
    std::string_view msg,
    unsigned long const code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << std::hex << code << std::dec
        << std::endl;
}

template <typename Func>
Func resolve_native_api(char const* function_name)
{
    return reinterpret_cast<Func>(::GetProcAddress(
            ::GetModuleHandleW(L"ntdll"), function_name));
}

std::wstring construct_full_object_name(
    unsigned long pid, 
    char const*   name
    )
{
    wchar_t buffer[OBJECT_NAME_BUFLEN];
    auto session_id = unsigned long{};

    ::ProcessIdToSessionId(pid, &session_id);

    auto count = swprintf_s(
        buffer, 
        L"\\Sessions\\%u\\BaseNamedObjects\\%hs", 
        session_id, name);

    return std::wstring(buffer, count);
}

std::optional<unsigned long> get_target_pid(char const* process_image)
{
    wchar_t process_image_wide[IMAGE_BUFLEN];
    auto converted = size_t{};
    
    mbstowcs_s(&converted, process_image_wide, process_image, IMAGE_BUFLEN/sizeof(wchar_t)-1);

    auto snap = invalid_handle{
        ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) };
    if (!snap)
    {
        return std::nullopt;
    }

    auto entry = PROCESSENTRY32W{};
    entry.dwSize = sizeof(PROCESSENTRY32W);

    ::Process32FirstW(snap.get(), &entry);

    std::optional<unsigned long> pid = std::nullopt;
    while (::Process32NextW(snap.get(), &entry))
    {
        if (_wcsicmp(entry.szExeFile, process_image_wide) == 0)
        {
            // found it
            pid = entry.th32ProcessID;
            break;
        }
    }

    return pid;
}

std::unique_ptr<unsigned char[]> 
enumerate_remote_handles(HANDLE target_process)
{
    // resolve NtQueryInformationProcess() from native API
    auto nt_query_process = resolve_native_api<nt_query_process_f>("NtQueryInformationProcess");
    if (nullptr == nt_query_process)
    {
        std::cout << "[-] Failed to resolve NtQueryInformationProcess()\n";
        return std::unique_ptr<unsigned char[]>{};
    }

    // enumerate the handles in the target process
    auto size = unsigned long{1 << 10};
    std::unique_ptr<unsigned char[]> buffer;
    for (;;)
    {
        buffer = std::make_unique<unsigned char[]>(size);
        auto status = nt_query_process(
            target_process,
            ProcessHandleInformation,
            buffer.get(),
            size,
            &size);
        if (STATUS_INFO_LENGTH_MISMATCH == status)
        {
            size += 1 << 10;
            continue;
        }

        if (!NT_SUCCESS(status))
        {
            // some other error occurred; invalidate the pointer
            buffer.release();
        }

        break;
    }

    return buffer;
}

bool process_handles_and_close_target(
    std::unique_ptr<unsigned char[]>& handle_info, 
    HANDLE                            target_process,
    std::wstring const&               object_name
    )
{
    auto found_it = false;

    auto nt_query_object = resolve_native_api<nt_query_object_f>("NtQueryObject");
    if (nullptr == nt_query_object)
    {
        std::cout << "[-] Failed to resolve NtQueryObject()\n";
        return found_it;
    }

    // iterate over all handles in the target process
    auto info = reinterpret_cast<PROCESS_HANDLE_SNAPSHOT_INFORMATION*>(handle_info.get());
    
    std::cout << "[+] Processing " << info->NumberOfHandles << " handles...\n";

    for (auto i = 0u; i < info->NumberOfHandles; ++i)
    {
        std::cout << "iter = " << i << '\n';

        auto h = info->Handles[i].HandleValue;
        auto target = HANDLE{};

        // duplicate the handle to the current process context
        if (!::DuplicateHandle(
            target_process, 
            h, 
            ::GetCurrentProcess(), 
            &target,
            0, FALSE,
            DUPLICATE_SAME_ACCESS))
        {
            continue;
        }

        // query the name of the underlying object
        auto name_buffer = std::make_unique<unsigned char[]>(1 << 10);
        auto status = nt_query_object(
            target, 
            ObjectNameInformation, 
            name_buffer.get(),
            1 << 10,
            nullptr);

        // once query is complete, no longer need duplicated handle
        ::CloseHandle(target);

        if (!NT_SUCCESS(status))
            continue;

        auto name = reinterpret_cast<UNICODE_STRING*>(name_buffer.get());
        std::wcout << std::wstring_view{name->Buffer, name->Length} << std::endl;
        if (name->Buffer &&
            _wcsnicmp(name->Buffer, object_name.c_str(), object_name.length()) == 0)
        {
            // found the target handle; duplicate again with appropriate flags
            // to close the handle in the source process
            ::DuplicateHandle(
                target_process,
                h,
                ::GetCurrentProcess(),
                &target,
                0, FALSE,
                DUPLICATE_CLOSE_SOURCE);
            
            ::CloseHandle(target);
            
            found_it = true;
            break;
        }
    }

    return found_it;
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "[-] Invalid arguments\n"
            << "[-] Usage: " << argv[0] << "<PROCESS IMAGE> <OBJECT NAME>\n";
        return STATUS_FAILURE_I;
    }

    auto& process_image = argv[1];
    auto& object_name   = argv[2];

    // determine the PID of the target process
    auto target_pid = get_target_pid(process_image);
    if (!target_pid.has_value())
    {
        std::cout << "[-] Failed to locate target process\n";
        return STATUS_FAILURE_I;
    }

    std::cout << "[+] Successfully located target process\n"
        << "\tImage: " << std::string_view{process_image, strlen(process_image)}
        << "\n\tPID: " << target_pid.value() << '\n';

    // acquire a handle to the target process
    auto target_process = null_handle{
        ::OpenProcess(
            PROCESS_QUERY_INFORMATION | 
            PROCESS_DUP_HANDLE, 
            FALSE, target_pid.value())};
    if (!target_process)
    {
        error("OpenProcess() failed");
        return STATUS_FAILURE_I;
    }

    std::cout << "[+] Successfully acquired handle to target process\n";

    auto handle_info = enumerate_remote_handles(target_process.get());
    if (!handle_info)
    {
        std::cout << "[+] Failed to enumerate remote process handles\n";
        return STATUS_FAILURE_I;
    }

    std::cout << "[+] Successfully enumerated remote process handles\n";

    // construct the full object name
    auto full_object_name 
        = construct_full_object_name(target_pid.value(), object_name);

    if (process_handles_and_close_target(
        handle_info,
        target_process.get(),
        full_object_name))
    {
        std::cout << "[+] Successfully closed target handle\n";
    }
    else
    {
        std::cout << "[-] Failed to close target handle\n";
    }

    return STATUS_SUCCESS_I;
}