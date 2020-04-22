// interprocess_queue.hpp
//
// Threadsafe interprocess queue implementation.

#pragma once

#include <windows.h>

// queue_msg_t is simply a standing for a generic storage type
struct queue_msg_t
{
    unsigned long id;
};

struct queue_t
{
    HANDLE m_guard;
    HANDLE m_event_notempty;
    HANDLE m_event_notfull;
    
    HANDLE m_mapping;
    char*  m_buffer;

    constexpr static const auto MAX_NAME_LEN    = 64;
    constexpr static const auto NAME_BUFFER_LEN = 128;
};

// queue_initialize
//
// Initialize a new queue object.
bool queue_initialize(
    queue_t*       q, 
    wchar_t const* name, 
    unsigned long  capacity
    );

// queue_destroy
//
// Release the resources internal to the queue object.
void queue_destroy(queue_t* q);

// queue_push_back
//
// Push new data onto the queue.
bool queue_push_back(queue_t* q, queue_msg_t* msg);

// queue_pop_front
//
// Pop data off the queue.
bool queue_pop_front(queue_t* q, queue_msg_t* msg);