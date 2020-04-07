// symlinks.cpp
//
// Query symbolic links with QueryDosDevice()
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 symlinks.cpp

#include <windows.h>
#include <string.h>

#include <set>
#include <tuple>
#include <string>
#include <memory>
#include <iostream>
#include <string_view>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

using link_pair = std::pair<std::string, std::string>;

void trace_info(std::string_view msg)
{
    std::cout << "[+] " << msg << '\n';
}

void trace_error(
    std::string_view msg,
    unsigned long code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << " Code: " << std::hex << code
        << std::dec << '\n';
}

int main(int argc, char* argv[])
{
    // allocate a sufficiently large buffer
    auto size = 1 << 10;
    std::unique_ptr<char[]> buffer;
    for (;;)
    {
        buffer = std::make_unique<char[]>(size);
        if (0 == ::QueryDosDeviceA(nullptr, buffer.get(), size))
        {
            auto err = ::GetLastError();
            if (ERROR_INSUFFICIENT_BUFFER == err)
            {
                // resize and try again
                size *= 2;
                continue;
            }
            else
            {
                trace_error("QueryDosDevice() failed", err);
                return STATUS_FAILURE_I;
            }
        }
        else
        {
            // success
            break;
        }
    }

    if (argc > 1)
    {
        // convert argument to lowercase
        ::_strlwr_s(argv[1], ::strlen(argv[1]) + 1);
    }

    auto const filter = argc > 1 ? argv[1] : nullptr;

    struct compare_lower
    {
        bool operator()(link_pair const& l, link_pair const& r) const
        {
            return ::_stricmp(l.first.c_str(), r.first.c_str()) < 0;
        }
    };

    auto links = std::set<link_pair, compare_lower>{};
    char target[512];

    for (auto p = buffer.get(); *p;)
    {
        auto name = std::string{p};
        
        auto lower = std::string{name};
        ::_strlwr_s(lower.data(), lower.length() + 1);

        if (filter == nullptr || lower.find(filter) != std::string::npos)
        {
            ::QueryDosDeviceA(name.c_str(), target, _countof(target));
            links.insert({name, target});
        }

        p += (name.size() + 1);
    }

    for (auto const& l : links)
    {
        std::cout << l.first.c_str() << " = " << l.second.c_str() << '\n';
    }
    std::cout << std::flush;

    return STATUS_SUCCESS_I;
}