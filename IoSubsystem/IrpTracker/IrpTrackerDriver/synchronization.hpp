// synchronization.hpp
//
// Synchronization helpers.

#pragma once

#include <ntddk.h>

class FastMutex
{
	FAST_MUTEX m_mutex;
public:
	void init()
	{
		::ExInitializeFastMutex(&m_mutex);
	}

	void lock()
	{
		::ExAcquireFastMutex(&m_mutex);
	}

	void unlock()
	{
		::ExReleaseFastMutex(&m_mutex);
	}
};

template<typename TLock>
class AutoLock
{
	TLock& m_lock;
public:
	AutoLock(TLock& lock)
		: m_lock(lock)
	{
		lock.lock();
	}

	~AutoLock()
	{
		m_lock.unlock();
	}
};
