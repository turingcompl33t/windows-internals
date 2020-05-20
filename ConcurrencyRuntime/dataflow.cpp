// dataflow.cpp
//
// Dataflow programming with asynchronous agents.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 dataflow.cpp

#include <windows.h>
#include <agents.h>
#include <iostream>
#include <random>

namespace crt = concurrency;

// synchronization primitive that is signaled when its count reaches zero
class countdown_event
{
    volatile long _current;

    crt::event _event;

    public:
    countdown_event(unsigned int count = 0L)
        : _current(static_cast<long>(count)) 
    {
        if (_current == 0L)
            _event.set();
    }

    countdown_event(countdown_event const&)            = delete;
    countdown_event& operator=(countdown_event const&) = delete;
        
    void signal() 
    {
        if(InterlockedDecrement(&_current) == 0L) 
        {
            _event.set();
        }
    }

    void add_count() 
    {
        if(InterlockedIncrement(&_current) == 1L) 
        {
            _event.reset();
        }
    }

    void wait() 
    {
        _event.wait();
    }
};

class control_flow_agent : public crt::agent
{
    crt::ISource<int>& _source;

    crt::single_assignment<std::size_t> _negatives;
    crt::single_assignment<std::size_t> _positives;

    public:
    explicit control_flow_agent(crt::ISource<int>& source)
        : _source(source)
    {}

    std::size_t negatives() 
    {
        return crt::receive(_negatives);
    }

    std::size_t positives()
    {
        return crt::receive(_positives);
    }

    protected:
    void run()
    {
        auto negative_count = std::size_t{0};
        auto positive_count = std::size_t{0};

        auto value = 0;      
        while ((value = crt::receive(_source)) != 0)
        {
            if (value < 0)
            ++negative_count;
            else
            ++positive_count;
        }

        send(_negatives, negative_count);
        send(_positives, positive_count);

        done();
    }
};

class dataflow_agent : public crt::agent
{
    crt::ISource<int>& _source;

    crt::single_assignment<size_t> _negatives;
    crt::single_assignment<size_t> _positives;

public:
    dataflow_agent(crt::ISource<int>& source)
        : _source(source)
    {}

    std::size_t negatives() 
    {
        return crt::receive(_negatives);
    }

    std::size_t positives()
    {
        return crt::receive(_positives);
    }

protected:
    void run()
    {
        auto negative_count = std::size_t{0};
        auto positive_count = std::size_t{0};

        auto active = countdown_event{};
        auto received_sentinel = crt::event{};
     
        auto increment_active = crt::transformer<int, int>(
            [&active](int value) -> int 
            {
                active.add_count();
                return value;
            });

        auto negatives = crt::call<int>(
            [&](int) 
            {
                ++negative_count;
                active.signal();
            },
            [](int value) -> bool 
            {
                return value < 0;
            });

        auto positives = crt::call<int>(
            [&](int) 
            {
                ++positive_count;
                active.signal();
            },
            [](int value) -> bool 
            {
                return value > 0;
            });

        auto sentinel = crt::call<int>(
            [&](int) 
            {            
            active.signal();
            received_sentinel.set();
            },
            [](int value) -> bool 
            { 
            return value == 0; 
            });

        auto connector = crt::unbounded_buffer<int>{};
        
        // connect the network

        connector.link_target(&negatives);
        connector.link_target(&positives);
        connector.link_target(&sentinel);
        increment_active.link_target(&connector);

        _source.link_target(&increment_active);

        // wait for the sentinel event and for all operations to finish
        received_sentinel.wait();
        active.wait();
            
        // write the counts to the message buffers
        send(_negatives, negative_count);
        send(_positives, positive_count);

        done();
    }
};

void send_values(
    crt::ITarget<int>& source, 
    int                sentinel, 
    std::size_t        count
    )
{
   auto gen = std::mt19937{42};
   for (std::size_t i = 0; i < count; ++i)
   {
      // Generate a random number that is not equal to the sentinel value.
      auto n = int{};
      while ((n = gen()) == sentinel);

      send(source, n);      
   }

   send(source, sentinel);   
}

int main()
{
    auto const sentinel = 0;
    std::size_t const count = 1000000;

    crt::unbounded_buffer<int> source;

    std::cout << "Control-flow agent:\n";

    control_flow_agent cf_agent(source);
    cf_agent.start();
   
    send_values(source, sentinel, count);
   
    crt::agent::wait(&cf_agent);
   
    std::cout << "There are " << cf_agent.negatives() 
            << " negative numbers\n";
    std::cout << "There are " << cf_agent.positives() 
            << " positive numbers\n";  

    std::cout << "Dataflow agent:\n";

    dataflow_agent df_agent(source);
    df_agent.start();
   
    send_values(source, sentinel, count);
    
    crt::agent::wait(&df_agent);

    std::cout << "There are " << df_agent.negatives() 
            << " negative numbers\n";
    std::cout << "There are " << df_agent.positives() 
            << " positive numbers\n";  
   
   return 0;
}