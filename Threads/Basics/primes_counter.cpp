// primes_counter.cpp
//
// Basic multithreading without synchronzation.
//
// Usage
//  primes_counter <N_THREADS> <FROM> <TO>
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 primes_counter.cpp

#include <windows.h>
#include <assert.h>

#include <tuple>
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <string_view>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

using args_t    = std::tuple<unsigned long, unsigned long long, unsigned long long>;
using results_t = std::pair<unsigned long, unsigned long long>; 

struct thread_data
{
    unsigned long long from;
    unsigned long long to;
    unsigned long      count;
};

void error(
    std::string_view msg,
    unsigned long code = ::GetLastError())
{
    std::cout << "[-] " << msg
        << " Code: " << code
        << std::endl;
}

std::optional<args_t> parse_arguments(char* arr[])
{
    unsigned long      n_threads;
    unsigned long long from;
    unsigned long long to;

    try
    {
        n_threads = std::stoul(arr[0]);
        from      = std::stoull(arr[1]);
        to        = std::stoull(arr[2]);
    }
    catch (std::exception const&)
    {
        return std::nullopt;
    }

    return std::make_tuple(n_threads, from, to);
}

bool is_prime(unsigned long long n)
{
    if (n < 3)
    {
        return false;
    }

    auto limit = static_cast<unsigned long long>(std::sqrt(n));
    for (auto i = 2; i < limit; ++i)
    {
        if (n % i == 0)
        {
            return false;
        }
    }

    return true;
}

unsigned long __stdcall count_primes(void* arg)
{
    auto data = static_cast<thread_data*>(arg);

    auto count = unsigned long{};
    auto& from = data->from;
    auto& to   = data->to;

    for (auto i = from; i < to; ++i)
    {
        if (is_prime(i))
        {
            ++count;
        }
    }

    data->count = count;
    return 0;
}

results_t count_all_primes(
    unsigned long n_threads,
    unsigned long long from,
    unsigned long long to
    )
{
    auto const start = ::GetTickCount64();

    auto data = std::make_unique<thread_data[]>(n_threads);
    auto handles = std::make_unique<HANDLE[]>(n_threads);

    auto const chunk_size = (to - from + 1) / n_threads;

    for (auto i = 0u; i < n_threads; ++i)
    {
        auto& d = data[i];
        d.from = i*chunk_size;
        d.to = i == (n_threads - 1) ? to : (i+1) * chunk_size - 1;

        auto id = unsigned long{};
        handles[i] = ::CreateThread(nullptr, 0, count_primes, &d, 0, &id);

        // lame error handling
        assert(handles[i]);
        std::cout << "[+] Thread " << i+1 << " created; TID = " << id << '\n';
    }

    ::WaitForMultipleObjects(n_threads, handles.get(), TRUE, INFINITE);
    auto elapsed = ::GetTickCount64() - start;

    auto user   = FILETIME{};
    auto kernel = FILETIME{};
    auto dummy  = FILETIME{};
    auto total_count = unsigned long{}; 

    for (auto i = 0u; i < n_threads; ++i)
    {
        ::GetThreadTimes(handles[i], &dummy, &dummy, &kernel, &user);

        auto count = data[i].count;
        auto time  = (user.dwLowDateTime + kernel.dwLowDateTime) / 10000;

        std::cout << "[+] Thread " << i+1
            << " count: " << count
            << " execution time: " << time << " ms"
            << '\n';

        total_count += count;
        ::CloseHandle(handles[i]);
    }

    return std::make_pair(total_count, elapsed);
}

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cout << "[-] Invalid arguments\n";
        std::cout << "Usage: " << argv[0]
            << " <N_THREADS> <FROM> <TO>\n";
        return STATUS_FAILURE_I;
    }

    auto opt_args = parse_arguments(argv+1);
    if (!opt_args.has_value())
    {
        std::cout << "Invalid arguments; failed to parse\n";
        return STATUS_FAILURE_I;
    }

    auto n_threads = std::get<0>(opt_args.value());
    auto from = std::get<1>(opt_args.value());
    auto to = std::get<2>(opt_args.value());

    if (n_threads < 1 || n_threads > 64 || from < 1 || to < 1 || to < from)
    {
        std::cout << "Invalid arguments; logially incoherent\n";
        return STATUS_FAILURE_I;
    }

    auto results = count_all_primes(n_threads, from, to);
    std::cout << "[+] Total Count: " << results.first << '\n'
              << "[+] Total Elapsed Time: " << results.second << " ms"
              << std::endl;

    return STATUS_SUCCESS_I;
}