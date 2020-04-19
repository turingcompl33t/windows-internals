// wait_on_address.cpp
//
// Demonstration of new(er) Windows WaitOnAddress synchronization primitive.
//
// Build
//  cl /EHsc /nologo /W4 /std:c++17 wait_on_address.cpp

#include <windows.h>

#include <chrono>
#include <thread>
#include <iostream>

#pragma comment(lib, "synchronization.lib")

// the address of interest
unsigned long g_target_value = 0;

// the thread that waits on address
void waiter()
{
    unsigned long undesired_value = 0;
    unsigned long captured_value = g_target_value;
    while (captured_value == undesired_value)
    {
        WaitOnAddress(&g_target_value, &undesired_value, sizeof(unsigned long), INFINITE);
        captured_value = g_target_value;
    }

    std::cout << "WaitOnAddress() satisifed: g_target_value = ";
    std::cout << g_target_value << '\n';
}

// the thread that modifies the address
void setter()
{
    using namespace std::chrono_literals;

    // wait a bit
    std::this_thread::sleep_for(3s);

    InterlockedIncrement(&g_target_value);
    WakeByAddressSingle(&g_target_value);
}

int main()
{
    auto t1 = std::thread{waiter};
    auto t2 = std::thread{setter};

    t1.join();
    t2.join();
}

