// init_once.cpp
//
// Basic usage of Windows init-once initialization.
// 
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\Catch2 init_once.cpp

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <windows.h>

#include <thread>
#include <vector>

constexpr const auto N_THREADS = 5;

struct Data
{
    unsigned long long n;
};

INIT_ONCE g_init = INIT_ONCE_STATIC_INIT;

BOOL __stdcall invoked_once(
    PINIT_ONCE /* init_once */,
    void*      param,
    void**     /* context */
    )
{
    auto& data = *static_cast<Data*>(param);
    ++data.n;

    return TRUE;
}

TEST_CASE("Function provided to InitOnceExecuteOnce() is indeed only executed once")
{
    auto data = Data{0};

    auto threads = std::vector<std::thread>{};
    for (auto i = 0u; i < N_THREADS; ++i)
    {
        threads.emplace_back([&data]()
        {
            ::InitOnceExecuteOnce(&g_init, invoked_once, &data, nullptr);
        });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    REQUIRE(data.n == 1);
}