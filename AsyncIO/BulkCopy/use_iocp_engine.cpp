// use_iocp_engine.cpp
//
// Refactor of bulk copy application to use iocp_copy_engine class implementation.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include use_iocp_engine.cpp

#include <chrono>
#include <optional>
#include <iostream>

#include "iocp_copy_engine.hpp"

std::optional<unsigned long> parse_threadcount(char* count_str)
{
    try
    {
        return std::stoul(count_str);
    }   
    catch (std::exception const&)
    {
        return std::nullopt;
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Invalid arguments\n";
        std::cout << "Usage: " << argv[0] << " <THREAD COUNT>\n";
        return STATUS_FAILURE_I;
    }

    // parse threadcount, default to hardware concurrency count
    auto threadcount = parse_threadcount(argv[1])
        .value_or(std::thread::hardware_concurrency());

    auto engine = iocp_copy_engine{threadcount};
    engine.add_file("src.txt", "dst.txt");
    engine.finalize();

    auto start = std::chrono::steady_clock::now();

    // blocks until completion
    engine.start();

    auto stop = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

    std::cout << "Completed in " << elapsed.count() << " ms\n";

    return STATUS_SUCCESS_I;
}