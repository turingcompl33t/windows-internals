// test_interprocess.cpp
//
// Test harness for interprocess queue.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 test_interprocess.cpp interprocess_queue.cpp

#include <iostream>
#include <string_view>

#include "interprocess_queue.hpp"

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto NAME_BUFFER_LEN = 128;

constexpr const auto QUEUE_CAPACITY  = 32;
constexpr const auto OPS_PER_PROCESS = 100;

void error(
    std::string_view msg,
    unsigned long const code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << "Code: " << std::hex << code << std::dec
        << std::endl;
}

unsigned long producer(queue_t& q)
{
    auto count = unsigned long{};
    for (auto i = 0u; i < OPS_PER_PROCESS; ++i)
    {
        auto msg = queue_msg_t{};
        msg.id = i;

        if (queue_push_back(&q, &msg))
        {
            ++count;
        }
    }

    return count;
}

unsigned long consumer(queue_t& q)
{
    auto count = unsigned long{};
    for (auto i = 0u; i < OPS_PER_PROCESS; ++i)
    {
        auto msg = queue_msg_t{};
        if (queue_pop_front(&q, &msg))
        {
            ++count;
        }
    }

    return count;
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "[-] Invalid arguments\n"
            << "[-] Usage: " << argv[0] << " <QUEUE NAME> <{p,c}>\n";
        return STATUS_FAILURE_I;
    }

    wchar_t name_buffer[NAME_BUFFER_LEN];
    auto converted = size_t{};
    ::mbstowcs_s(&converted, name_buffer, argv[1], NAME_BUFFER_LEN-1);

    // consume by default, I suppose
    auto const should_produce = ::strcmp(argv[2], "p") == 0;

    auto queue = queue_t{};

    if (!::queue_initialize(&queue, name_buffer, QUEUE_CAPACITY))
    {
        error("Failed to initialize queue");
        return STATUS_FAILURE_I;
    }

    std::cout << "[+] Successfully initialized queue\n"
        << "ENTER to begin " << (should_produce ? "producing" : "consuming") << '\n';
    std::cin.get();

    auto const count = should_produce ? producer(queue) : consumer(queue);

    std::cout << "[+] " << (should_produce ? "produced " : "consumed ")
        << count << " messages\n";

    queue_destroy(&queue);

    return STATUS_FAILURE_I;
}