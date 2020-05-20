// com_client.cpp
//
// Build
//	cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include com_client.cpp /Fe:client.exe

#include <wrl.h>
#include <wdl/error/com_exception.hpp>

#include <iostream>

#include "hen.h"  // only necessary for remoting
#include "com_server.hpp"

namespace err = wdl::error;
using namespace Microsoft::WRL;

enum class apartment
{
	multithreaded = COINIT_MULTITHREADED,
	singlethreaded = COINIT_APARTMENTTHREADED
};

struct com_runtime
{
	explicit com_runtime(apartment type = apartment::multithreaded)
	{
		err::check_com(::CoInitializeEx(
			nullptr,
			static_cast<unsigned long>(type)));
	}

	~com_runtime()
	{
		::CoUninitialize();
	}
};

struct event_handler : RuntimeClass<RuntimeClassFlags<ClassicCom>,
								    IAsyncHenEventHandler>
{
	HRESULT __stdcall OnCluck() override
	{
		std::cout << "Async Cluck!\n";
		return S_OK;
	}
};

void basics()
{
	com_runtime rt{};

	ComPtr<IHen> hen;

	/*
	ComPtr<IClassFactory> hatchery;

	err::check_com(::CoGetClassObject(
		__uuidof(Hen),
		CLSCTX_INPROC_SERVER,
		nullptr,
		__uuidof(hatchery),
		reinterpret_cast<void**>(hatchery.GetAddressOf())
	));

	err::check_com(hatchery->CreateInstance(
		nullptr,
		__uuidof(hen),
		reinterpret_cast<void**>(hen.GetAddressOf())
	));

	*/

	// the call here is functionally equivalent to the
	// code above but leverages the class factory interface
	// to simplify the code

	err::check_com(::CoCreateInstance(
		__uuidof(Hen),
		nullptr,
		CLSCTX_INPROC_SERVER,
		__uuidof(hen),
		reinterpret_cast<void**>(hen.GetAddressOf())));

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

int main()
{
	auto rt = com_runtime{apartment::singlethreaded};

	ComPtr<IUnknown> hen{};

	err::check_com(::CoCreateInstance(
		__uuidof(Hen),
		nullptr,
		CLSCTX_INPROC_SERVER,
		__uuidof(hen),
		reinterpret_cast<void**>(hen.GetAddressOf())));

	ComPtr<IHen> local{};

	if (S_OK == hen.CopyTo(local.GetAddressOf()))
	{
		std::cout << "Local\n";
	}

	ComPtr<IAsyncHen> async{};

	if (S_OK == hen.CopyTo(async.GetAddressOf()))
	{
		auto handler = Make<event_handler>();

		err::check_com(async->SetEventHandler(handler.Get()));
	}

	::Sleep(INFINITE);
}