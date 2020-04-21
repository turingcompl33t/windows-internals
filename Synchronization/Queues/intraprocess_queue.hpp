// intraprocess_queue.hpp
//
// Threadsafe intraprocess queue implementation.

#pragma once

#include <windows.h>

struct queue_msg_t
{
    unsigned long id;
};

struct queue_t
{
    SRWLOCK            m_guard;
    CONDITION_VARIABLE m_cond_notempty;
    CONDITION_VARIABLE m_cond_notfull;

    unsigned long m_capacity;
    unsigned long m_front;
    unsigned long m_back;

    queue_msg_t*  m_buffer;
};

// queue_initialize
//
// Initialize a new queue object.
bool queue_initialize(queue_t* q, unsigned long capacity);

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