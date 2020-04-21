// test_intraprocess.cpp
//
// Test harness for intraprocess queue.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 test_intraprocess.cpp intraprocess_queue.cpp

#include <thread>
#include <vector>
#include <atomic>
#include <cassert>
#include <iostream>

#include "intraprocess_queue.hpp"

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto QUEUE_CAPACITY = 32;
constexpr const auto N_THREADS      = 2;
constexpr const auto OPS_PER_THREAD = 100;

void producer(queue_t& q, std::atomic_uint& count)
{
    for (auto i = 0u; i < OPS_PER_THREAD; ++i)
    {
        auto msg = queue_msg_t{};
        msg.id = i;

        if (queue_push_back(&q, &msg))
        {
            ++count;
        }
    }
}

void consumer(queue_t& q, std::atomic_uint& count)
{
    for (auto i = 0u; i < OPS_PER_THREAD; ++i)
    {   
        auto msg = queue_msg_t{};
        if (queue_pop_front(&q, &msg))
        {
            ++count;
        }
    }
}

int main()
{
    auto queue = queue_t{};
    if (!queue_initialize(&queue, QUEUE_CAPACITY))
    {
        std::cout << "[-] Failed to initialize queue\n";
        return STATUS_FAILURE_I;
    }

    auto produce_count = std::atomic_uint{};
    auto consume_count = std::atomic_uint{};

    // spin up the threads
    auto threads = std::vector<std::thread>{};
    for (auto i = 0u; i < N_THREADS; ++i)
    {
        threads.emplace_back(producer, std::ref(queue), std::ref(produce_count));
        threads.emplace_back(consumer, std::ref(queue), std::ref(consume_count));
    }

    // wait for the threads to complete
    for (auto& t : threads)
    {
        t.join();
    }

    // verify results
    assert(produce_count == N_THREADS*OPS_PER_THREAD);
    assert(consume_count == N_THREADS*OPS_PER_THREAD);

    std::cout << "[+] Success!\n";

    queue_destroy(&queue);

    return STATUS_SUCCESS_I;
}