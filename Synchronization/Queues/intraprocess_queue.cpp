// intraprocess_queue.cpp
//
// Threadsafe intraprocess queue implementation.

#pragma once

#include <windows.h>
#include <atlbase.h>

#include "intraprocess_queue.hpp"

#ifdef _DEBUG
#define ASSERT _ASSERTE
#define VERIFY ASSERT
#define VERIFY_(result, expression) ASSERT(result == expression)
#else
#define ASSERT __noop
#define VERIFY(expression) (expression)
#define VERIFY_(result, expression) (expression)
#endif

// ----------------------------------------------------------------------------
//  Internal Definitions

bool queue_empty_unsafe(queue_t* q)
{
    ASSERT(q);

    return q->m_front == q->m_back;
}

bool queue_full_unsafe(queue_t* q)
{
    ASSERT(q);

    return (q->m_back + 1) % q->m_capacity == q->m_front;
}

void queue_insert_unsafe(queue_t* q, queue_msg_t* msg)
{
    ASSERT(q);
    ASSERT(msg);

    auto const next = q->m_back+1 >= q->m_capacity 
        ? 0 : q->m_back+1;

    auto* base = reinterpret_cast<char*>(q->m_buffer);
    ::memcpy_s(
        base + (q->m_back*sizeof(queue_msg_t)),
        sizeof(queue_msg_t),
        msg,
        sizeof(queue_msg_t));

    q->m_back = next;
}

void queue_remove_unsafe(queue_t* q, queue_msg_t* msg)
{
    ASSERT(q);
    ASSERT(msg);

    auto const next = q->m_front+1 >= q->m_capacity
        ? 0 : q->m_front+1;

    auto* base = reinterpret_cast<char*>(q->m_buffer);
    ::memcpy_s(
        msg,
        sizeof(queue_msg_t),
        base + (q->m_front*sizeof(queue_msg_t)),
        sizeof(queue_msg_t));

    q->m_front = next;
}

// ----------------------------------------------------------------------------
//  Exported Definitions

bool queue_initialize(queue_t* q, unsigned long capacity)
{
    if (0 == capacity)
        return false;

    q->m_buffer = static_cast<queue_msg_t*>(
        ::calloc(capacity + 1, sizeof(queue_msg_t)));
    if (!q->m_buffer)
        return false;

    ::InitializeSRWLock(&q->m_guard);
    ::InitializeConditionVariable(&q->m_cond_notempty);
    ::InitializeConditionVariable(&q->m_cond_notfull);

    q->m_capacity = capacity;
    q->m_front = 0;
    q->m_back  = 0;

    return true;
}

void queue_destroy(queue_t* q)
{
    if (q)
    {
        if (q->m_buffer)
        {
            ::free(q->m_buffer);
        }
    }
}
bool queue_push_back(queue_t* q, queue_msg_t* msg)
{
    if (!q || !msg)
        return false;

    ::AcquireSRWLockExclusive(&q->m_guard);

    while (queue_full_unsafe(q))
    {
        ::SleepConditionVariableSRW(&q->m_cond_notfull, &q->m_guard, INFINITE, 0);
    }   

    // perform the insertion
    queue_insert_unsafe(q, msg);

    ::WakeConditionVariable(&q->m_cond_notempty);
    ::ReleaseSRWLockExclusive(&q->m_guard);

    return true;
}

bool queue_pop_front(queue_t* q, queue_msg_t* msg)
{
    if (!q || !msg)
        return false;

    ::AcquireSRWLockExclusive(&q->m_guard);

    while (queue_empty_unsafe(q))
    {
        ::SleepConditionVariableSRW(&q->m_cond_notempty, &q->m_guard, INFINITE, 0);
    }

    // perform the removal
    queue_remove_unsafe(q, msg);

    ::WakeConditionVariable(&q->m_cond_notfull);
    ::ReleaseSRWLockExclusive(&q->m_guard);

    return true;
}