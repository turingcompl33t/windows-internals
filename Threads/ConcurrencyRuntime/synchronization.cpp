// synchronization.cpp
// Demonstration of PPL cooperative synchronization.

#include <windows.h>
#include <crtdbg.h>
#include <ppl.h>

#include <cstdio>

#define ASSERT _ASSERTE

using namespace concurrency;

// IMPT: critical_section is merely one of the
// synchronization types defined by the concurrency
// runtime; the important distinction between these
// primitives and the primitives provided by the OS
// is that these primitives are cooperative and are
// only meant to be used on top of the runtime; 
// trying to utilize the critical_section shown
// below to synchronize standard windows threads
// created with CreateThread() would result in very
// poor performance

int wmain()
{
	critical_section cs{};
	
	cs.lock();
	// protected
	cs.unlock();

	if (cs.try_lock())
	{
		// protected
		cs.unlock();
	}

	{
		critical_section::scoped_lock guard(cs);
		// protected
	}

	return 0;
}