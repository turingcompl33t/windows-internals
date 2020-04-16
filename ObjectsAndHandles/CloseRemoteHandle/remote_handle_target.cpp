// remote_handle_target.cpp
//
// Target application for remote handle closure.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include remote_handle_target.cpp

#include <windows.h>
#include <wdl/handle/generic.hpp>

#include <string.h>
#include <iostream>
#include <string_view>

using null_handle = wdl::handle::null_handle;

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto EVENT_NAME_BUFLEN = 128;

void error(
    std::string_view msg,
    unsigned long const code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << std::hex << code << std::dec
        << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "[-] Invalid arguments\n"
            << "[-] Usage: " << argv[0] << " <EVENT NAME>\n";
        return STATUS_FAILURE_I;
    }

    auto& event_name = argv[1];

    // create a named event object
    auto event = null_handle{
        ::CreateEventA(nullptr, TRUE, FALSE, event_name) };
    if (!event)
    {
        error("CreateEvent() failed");
        return STATUS_FAILURE_I;
    }

    std::cout << "[+] Successfully created named event: "
        << std::string_view{event_name, strlen(event_name)} << '\n';
    
    // wait for user input 
    std::cin.get();

    return STATUS_SUCCESS_I;
}