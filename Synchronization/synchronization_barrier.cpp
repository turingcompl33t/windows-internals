// synchronization_barrier.cpp
//
// Demonstration of new(er) Windows SYNCHRONIZATION_BARRIER primitive.
//
// Build
//  cl /EHsc /nologo /W4 /std:c++17 /I %WIN_WORKSPACE%\_Deps\CDL\include synchronization_barrier.cpp

#include <windows.h>

#include <thread>
#include <chrono>
#include <vector>
#include <iostream>

#include "cdl/timing/uniform_wait.hpp"
#include "cdl/synchronization/monitor.hpp"

#pragma comment(lib, "synchronization.lib")

using cdl::timing::uniform_sec_wait;
using cdl::synchronization::monitor;

constexpr auto N_THREADS = 4;

monitor<std::ostream&> sync_cout{std::cout};

void thread_fn(
    unsigned id, 
    LPSYNCHRONIZATION_BARRIER barrier
    )
{
    // generate random 1-5 second durations
    auto waiter = uniform_sec_wait{1, 5};

    sync_cout([=](std::ostream& cout){ cout << "[" << id << "] Started Phase 1\n"; });

    // do some work
    std::this_thread::sleep_for(waiter.next());

    sync_cout([=](std::ostream& cout){ cout << "[" << id << "] Finished Phase 1\n"; });

    EnterSynchronizationBarrier(barrier, 0);

    sync_cout([=](std::ostream& cout){ cout << "[" << id << "] Started Phase 2\n"; });

    // do some work
    std::this_thread::sleep_for(waiter.next());

    sync_cout([=](std::ostream& cout){ cout << "[" << id << "] Finished Phase 2\n"; });

    EnterSynchronizationBarrier(barrier, 0);
}

int main()
{
    auto barrier = SYNCHRONIZATION_BARRIER{};
    InitializeSynchronizationBarrier(&barrier, N_THREADS, -1);  // default spin count

    std::vector<std::thread> threads{};
    for (unsigned id = 0; id < N_THREADS; ++id)
    {
        threads.emplace_back(thread_fn, id, &barrier);
    }

    // wait for all threads to complete 
    for (auto& t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
}

