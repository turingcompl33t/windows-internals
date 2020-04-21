// com_client.cpp
//
// Build
//	cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include com_client.cpp

#include <wrl.h>
#include <wdl/error/com_exception.hpp>

#include "com_server.hpp"

using namespace wdl::error;
using namespace Microsoft::WRL;

struct ComRuntime
{
	ComRuntime()
	{
		check_com(::CoInitializeEx(
			nullptr,
			COINITBASE_MULTITHREADED
		));
	}

	~ComRuntime()
	{
		::CoUninitialize();
	}
};

int main()
{
	ComRuntime rt{};

	ComPtr<IHen> hen;

	/*
	ComPtr<IClassFactory> hatchery;

	check_com(::CoGetClassObject(
		__uuidof(Hen),
		CLSCTX_INPROC_SERVER,
		nullptr,
		__uuidof(hatchery),
		reinterpret_cast<void**>(hatchery.GetAddressOf())
	));

	check_com(hatchery->CreateInstance(
		nullptr,
		__uuidof(hen),
		reinterpret_cast<void**>(hen.GetAddressOf())
	));

	*/

	// the call here is functionally equivalent to the
	// code above but leverages the class factory interface
	// to simplify the code

	check_com(::CoCreateInstance(
		__uuidof(Hen),
		nullptr,
		CLSCTX_INPROC_SERVER,
		__uuidof(hen),
		reinterpret_cast<void**>(hen.GetAddressOf())
		));

	hen->Cluck();

	// **client application controls unloading of server DLL**
	
	// these two calls are equivalent
	// ::CoFreeUnusedLibraries();
	// ::CoFreeUnusedLibrariesEx(INFINITE, 0);
	
	// "immediately unload all libraries which can be unloaded"
	// unloadability of COM server determined by COM runtime's 
	// call to the DllCanUnloadNow() function
	//
	// on this first call, the COM server denies immediate
	// unloading because there are outstanding references 
	::CoFreeUnusedLibrariesEx(0, 0);

	// release the hen 
	hen.Reset();

	// now, because all references have been released,
	// the runtime can unload the server DLL from our process
	::CoFreeUnusedLibrariesEx(0, 0);
}