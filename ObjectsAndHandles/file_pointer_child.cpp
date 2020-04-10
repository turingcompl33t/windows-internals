// file_pointer_child.cpp
//
// Interaction of handling inheritance and file pointer.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include file_pointer_child.cpp

#include <windows.h>
#include <string.h>

#include <string>
#include <optional>
#include <iostream>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

void error(
    std::string const& msg,
    unsigned long code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << "Code: " << std::hex << code << std::dec
        << std::endl;
}

std::optional<unsigned long> parse_handle_number(char* arr[])
{
    try
    {
        return std::stoul(arr[0]);
    }
    catch (std::exception const&)
    {
        return std::nullopt;
    }
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

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Invalid arguments\n";
        std::cout << "Usage: " << argv[0] << "<HANDLE VALUE>\n";
        return STATUS_FAILURE_I;
    }

    auto handle_no = parse_handle_number(argv+1);
    if (!handle_no.has_value())
    {
        std::cout << "Failed to parse handle number\n";
        return STATUS_FAILURE_I;
    }    

    auto file = ::ULongToHandle(handle_no.value());

    auto offset = get_file_pointer(file);
    if (!offset.has_value())
    {
        error("GetFilePointer() failed");
        return STATUS_FAILURE_I;
    }

    std::cout << "[CHILD] Offset = " << offset.value() << '\n';

    if (!set_file_pointer(file, 100))
    {
        error("SetFilePointer() failed");
        return STATUS_FAILURE_I;
    }

    offset = get_file_pointer(file);
    if (!offset.has_value())
    {
        error("GetFilePointer() failed");
        return STATUS_FAILURE_I;
    }

    std::cout << "[CHILD] Offset = " << offset.value() << '\n';
    std::cout << std::flush;

    return STATUS_SUCCESS_I;
}
