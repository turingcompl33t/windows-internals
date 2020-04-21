// interprocess_queue.cpp
//
// Threadsafe interprocess queue implementation.

#include <windows.h>
#include <cstdio>
#include <atlbase.h>

#include "interprocess_queue.hpp"

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

// queue_buffer_header_t defines the memory layout
// at the very beginning of the shared memory region
struct queue_buffer_header_t
{
    unsigned long capacity;
    unsigned long back;
    unsigned long front;
};

bool queue_empty_unsafe(queue_t* q)
{
    ASSERT(q);

    auto* header = reinterpret_cast<queue_buffer_header_t*>(q->m_buffer);

    return header->front == header->back;
}

bool queue_full_unsafe(queue_t* q)
{
    ASSERT(q);

    auto* header = reinterpret_cast<queue_buffer_header_t*>(q->m_buffer);

    return (header->back + 1) % header->capacity == header->front;
}

void queue_insert_unsafe(queue_t* q, queue_msg_t* msg)
{
    ASSERT(q);
    ASSERT(msg);

    auto* header = reinterpret_cast<queue_buffer_header_t*>(q->m_buffer);
    
    auto const next = header->back+1 >= header->capacity
        ? 0 : header->back+1;

    auto* base = reinterpret_cast<char*>(q->m_buffer + sizeof(queue_buffer_header_t));

    ::memcpy_s(
        base + (header->back * sizeof(queue_msg_t)), 
        sizeof(queue_msg_t), 
        msg, 
        sizeof(queue_msg_t));

    header->back = next;
}

void queue_remove_unsafe(queue_t* q, queue_msg_t* msg)
{
    ASSERT(q);
    ASSERT(msg);

    auto* header = reinterpret_cast<queue_buffer_header_t*>(q->m_buffer);

    auto const next = header->front+1 >= header->capacity
        ? 0 : header->front+1;

    auto* base = reinterpret_cast<char*>(q->m_buffer + sizeof(queue_buffer_header_t));

    ::memcpy_s(
        msg,
        sizeof(queue_msg_t),
        base + (header->front * sizeof(queue_msg_t)),
        sizeof(queue_msg_t));

    header->front = next;
}

// ----------------------------------------------------------------------------
//  Exported Definitions

bool queue_initialize(
    queue_t*       q, 
    wchar_t const* name, 
    unsigned long  capacity
    )
{
    if (!q)
        return false;

    auto const len = ::wcsnlen_s(name, queue_t::MAX_NAME_LEN);
    if (0 == len || queue_t::MAX_NAME_LEN == len)
        return false;

    wchar_t guard_name_buffer[queue_t::NAME_BUFFER_LEN];
    wchar_t event_ne_name_buffer[queue_t::NAME_BUFFER_LEN];
    wchar_t event_nf_name_buffer[queue_t::NAME_BUFFER_LEN];
    wchar_t mapping_name_buffer[queue_t::NAME_BUFFER_LEN];

    ::swprintf_s(guard_name_buffer,    L"%sguard", name);
    ::swprintf_s(event_ne_name_buffer, L"%sevent_notempty", name);
    ::swprintf_s(event_nf_name_buffer, L"%sevent_notfull", name);
    ::swprintf_s(mapping_name_buffer,  L"%smapping", name);

    // could use some error checking here...
    q->m_guard          = ::CreateMutexW(nullptr, FALSE, guard_name_buffer);
    q->m_event_notempty = ::CreateEventW(nullptr, FALSE, FALSE, event_ne_name_buffer);
    q->m_event_notfull  = ::CreateEventW(nullptr, FALSE, FALSE, event_nf_name_buffer);

    auto size = ULARGE_INTEGER{};
    size.QuadPart = (capacity+1)*sizeof(queue_msg_t) 
        + sizeof(queue_buffer_header_t);

    q->m_mapping = ::CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE | SEC_COMMIT,
        size.HighPart,
        size.LowPart,
        mapping_name_buffer);

    // only the first thread through the constructor should initialize
    // obviously this current approach is lacking...
    auto const should_initialize 
        = ::GetLastError() != ERROR_ALREADY_EXISTS;

    q->m_buffer = static_cast<char*>(
        ::MapViewOfFile(
            q->m_mapping, 
            FILE_MAP_READ | FILE_MAP_WRITE,
            0, 0,
            size.QuadPart));

    if (should_initialize)
    {
        auto* header = reinterpret_cast<queue_buffer_header_t*>(q->m_buffer);
        header->capacity = capacity;
        header->back  = 0;
        header->front = 0;
    }
    
    return true;
}

void queue_destroy(queue_t* q)
{
    if (q)
    {
        ::CloseHandle(q->m_guard);
        ::CloseHandle(q->m_event_notempty);
        ::CloseHandle(q->m_event_notfull);
        ::CloseHandle(q->m_mapping);
        ::UnmapViewOfFile(q->m_buffer);
    }
}

bool queue_push_back(queue_t* q, queue_msg_t* msg)
{
    if (!q || !msg)
        return false;

    ::WaitForSingleObject(q->m_guard, INFINITE);

    while (queue_full_unsafe(q))
    {
        ::SignalObjectAndWait(q->m_guard, q->m_event_notfull, INFINITE, FALSE);
        ::WaitForSingleObject(q->m_guard, INFINITE);
    }

    // perform the insertion
    queue_insert_unsafe(q, msg);

    ::SetEvent(q->m_event_notempty);
    ::ReleaseMutex(q->m_guard);

    return true;
}

bool queue_pop_front(queue_t* q, queue_msg_t* msg)
{
    if (!q || !msg)
        return false;

    ::WaitForSingleObject(q->m_guard, INFINITE);

    while (queue_empty_unsafe(q))
    {
        ::SignalObjectAndWait(q->m_guard, q->m_event_notempty, INFINITE, FALSE);
        ::WaitForSingleObject(q->m_guard, INFINITE);
    }

    // perform the removal
    queue_remove_unsafe(q, msg);

    ::SetEvent(q->m_event_notfull);
    ::ReleaseMutex(q->m_guard);

    return true;
}