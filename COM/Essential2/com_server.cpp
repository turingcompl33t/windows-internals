// com_server.cpp
//
// Build
//	cl /EHsc /nologo /std:c++17 /W4 /LD /I %WIN_WORKSPACE%\_Deps\WDL\include registration.cpp com_server.cpp

#include <windows.h>
#include <memory>
#include <ktmw32.h>
#pragma comment(lib, "ktmw32")

#include <atlbase.h>
#define ASSERT ATLASSERT
#define TRACE  ATLTRACE

#include "com_server.hpp"

// track total number of outstanding references
static long s_serverLock;

struct Hen : IHen
{
	long m_count;

	Hen() : m_count{ 0 }
	{
		_InterlockedIncrement(&s_serverLock);
	}

	~Hen()
	{
		_InterlockedDecrement(&s_serverLock);
	}

	ULONG __stdcall AddRef() override
	{
		return _InterlockedIncrement(&m_count);
	}

	ULONG __stdcall Release() override
	{
		const auto temp = _InterlockedDecrement(&m_count);

		if (0 == temp)
		{
			delete this;
		}

		return temp;
	}

	HRESULT __stdcall QueryInterface(
		const IID& id,
		void** result
	) override
	{
		ASSERT(result);

		if (id == __uuidof(IHen) ||
			id == __uuidof(IUnknown))
		{
			*result = static_cast<IHen*>(this);
		}
		else
		{
			*result = 0;
			return E_NOINTERFACE;
		}

		static_cast<IUnknown*>(*result)->AddRef();
		return S_OK;
	}

	void __stdcall Cluck() override
	{
		TRACE(L"Cluck!\n");
	}
};

struct Hatchery : IClassFactory
{
	ULONG __stdcall AddRef() override
	{
		return 2;
	}

	ULONG __stdcall Release() override
	{
		return 1;
	}

	HRESULT __stdcall QueryInterface(
		const IID& id,
		void** result
	) override
	{
		ASSERT(result);

		if (id == __uuidof(IClassFactory) ||
			id == __uuidof(IUnknown))
		{
			*result = static_cast<IClassFactory*>(this);
		}
		else
		{
			*result = 0;
			return E_NOINTERFACE;
		}

		return S_OK;
	}

	// CreateInstance
	// Required to implement IClassFactory.
	HRESULT __stdcall CreateInstance(
		IUnknown* outer,
		const IID& iid,
		void** result
	) override
	{
		ASSERT(result);
		*result = nullptr;

		if (outer)
		{
			// do not support aggregation
			return CLASS_E_NOAGGREGATION;
		}

		auto hen = new (std::nothrow) Hen;

		if (!hen)
		{
			return E_OUTOFMEMORY;
		}

		hen->AddRef();
		auto hr = hen->QueryInterface(iid, result);
		hen->Release();

		return hr;
	}

	// LockServer
	// Required to implement IClassFactory.
	HRESULT __stdcall LockServer(BOOL lock) override
	{
		if (lock)
		{
			_InterlockedIncrement(&s_serverLock);
		}
		else
		{
			_InterlockedDecrement(&s_serverLock);
		}

		return S_OK;
	}
};

// DllGetClassObject
HRESULT __stdcall DllGetClassObject(
	const CLSID& clsid,
	const IID& iid,
	void** result
	)
{
	ASSERT(result);
	*result = nullptr;

	if (clsid == __uuidof(Hen))
	{
		static Hatchery hatchery;
		return hatchery.QueryInterface(iid, result);
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

// DllCanUnloadNow
// Invoked by the runtime to determine if a particular
// instance of this COM server may be unloaded from the
// process in which it resides.
HRESULT __stdcall DllCanUnloadNow()
{
	return s_serverLock ? S_FALSE : S_OK;
}

BOOL __stdcall DllMain(
	HINSTANCE module,
	DWORD     reason,
	void*
)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		::DisableThreadLibraryCalls(module);
		TRACE(L"DllMain: LoadLibrary\n");
		break;
	case DLL_PROCESS_DETACH:
		TRACE(L"DllMain: FreeLibrary\n");
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	default:
		break;
	}

	return TRUE;
}