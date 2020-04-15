// share_inherit_child.cpp
//
// Demonstrates sharing of kernel object handle via inheritance.
//
// Build:
//  cl /EHsc /nologo /std:c++17 /W4 share_inherit_child.cpp

#include <windows.h>
#include <cstdio>
#include <string>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

int main(int, char* argv[])
{
    auto handle_num = unsigned long{};

    try
    {
        handle_num = std::stoul(argv[1]);
    }
    catch(std::exception const&)
    {
        // std::stoul() may throw 
        //  - std::invalid_argument
        //  - std::out_of_range 
        printf("[CHILD] Failed to parse arguments\n");
        return STATUS_FAILURE_I;
    }

    auto event = ::ULongToHandle(handle_num);

    printf("[CHILD] Got handle to event; waiting for signal\n");

    ::WaitForSingleObject(event, INFINITE);

    printf("[CHILD] Event signaled, exiting\n");
    
    return STATUS_SUCCESS_I;
}