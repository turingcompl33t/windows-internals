// registration.cpp

#include <windows.h>
#include <ktmw32.h>

#include <wdl/handle/generic.hpp>
#include <wdl/registry/base.hpp>

// Transaction
// Unique handle wrapper around transaction object.
using Transaction = wdl::handle::invalid_handle;

// RegistryKey
// Unique handle wrapper around registry handle manager.
using RegistryKey = wdl::registry::registry_handle;

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

enum class EntryOption
{
	None,
	Delete,
	FileName
};

struct Entry
{
	const wchar_t* Path;
	EntryOption    Option;
	const wchar_t* Name;
	const wchar_t* Value;
};

static Entry Table[] =
{
	{
		L"Software\\Classes\\CLSID\\{6d5f1802-781c-4b95-8aeb-f33d07adb5db}",
		EntryOption::Delete,
		nullptr,
		L"Hen"
	},
	{
		L"Software\\Classes\\CLSID\\{6d5f1802-781c-4b95-8aeb-f33d07adb5db}\\InprocServer32",
		EntryOption::FileName
	},
	{
		L"Software\\Classes\\CLSID\\{6d5f1802-781c-4b95-8aeb-f33d07adb5db}\\InprocServer32",
		EntryOption::None,
		L"ThreadingModel",
		L"Free"
	}
};

// CreateTransaction
// Wrapper around Win32 API for transaction creation.
// Returns unique handle wrapper around transaction object.
static Transaction CreateTransaction()
{
	return Transaction(::CreateTransaction(
		nullptr,
		nullptr,
		TRANSACTION_DO_NOT_PROMOTE,
		0, 0,
		INFINITE,
		nullptr
	));
}

// CreateRegistryKey
// Wrapper around transactional registry API.
// Returns unique handle wrapper around key handle.
static RegistryKey CreateRegistryKey(
	HKEY key,
	const wchar_t* path,
	const Transaction& txn,
	REGSAM access
)
{
	HKEY handle = nullptr;

	auto result = ::RegCreateKeyTransactedW(
		key,
		path,
		0,
		nullptr,
		REG_OPTION_NON_VOLATILE,
		access,
		nullptr,
		&handle,
		nullptr,
		txn.get(),
		nullptr
		);

	if (ERROR_SUCCESS != result)
	{
		::SetLastError(result);
	}

	return RegistryKey{ handle };
}

// OpenRegistryKey()
// Wrapper around transactional registry API.
// Returns unique handle wrapper around key handle.
static RegistryKey OpenRegistryKey(
	HKEY key,
	const wchar_t* path,
	const Transaction& txn,
	REGSAM access
)
{
	HKEY handle = nullptr;

	auto result = ::RegOpenKeyTransactedW(
		key,
		path,
		0,
		access,
		&handle,
		txn.get(),
		nullptr
	);

	if (ERROR_SUCCESS != result)
	{
		::SetLastError(result);
	}

	return RegistryKey{ handle };
}

static bool Unregister(const Transaction& txn)
{
	for (const auto& entry : Table)
	{
		if (EntryOption::Delete != entry.Option)
		{
			continue;
		}

		auto key = OpenRegistryKey(
			HKEY_LOCAL_MACHINE,
			entry.Path,
			txn,
			DELETE |
			KEY_ENUMERATE_SUB_KEYS |
			KEY_QUERY_VALUE |
			KEY_SET_VALUE
		);

		if (!key)
		{
			if (ERROR_FILE_NOT_FOUND == ::GetLastError())
			{
				continue;
			}

			return false;
		}

		auto result = ::RegDeleteTreeW(key.get(), nullptr);

		if (ERROR_SUCCESS != result)
		{
			::SetLastError(result);
			return false;
		}
	}

	return true;
}

static bool Register(const Transaction& txn)
{
	if (!Unregister(txn))
	{
		return false;
	}

	wchar_t filename[MAX_PATH];

	const auto length = ::GetModuleFileNameW(
		reinterpret_cast<HMODULE>(&__ImageBase),
		filename,
		_countof(filename)
		);

	if (0 == length || _countof(filename) == length)
	{
		return false;
	}

	for (const auto& entry : Table)
	{
		auto key = CreateRegistryKey(
			HKEY_LOCAL_MACHINE,
			entry.Path,
			txn,
			KEY_WRITE
		);

		if (!key)
		{
			return false;
		}

		if (EntryOption::FileName != entry.Option &&
			!entry.Value)
		{
			continue;
		}

		auto value = entry.Value;

		if (!value)
		{
			ASSERT(EntryOption::FileName == entry.Option);
			value = filename;
		}

		auto result = ::RegSetValueExW(
			key.get(),
			entry.Name,
			0,
			REG_SZ,
			reinterpret_cast<const BYTE*>(value),
			static_cast<DWORD>(sizeof(wchar_t) * (wcslen(value) + 1))
			);

		if (ERROR_SUCCESS != result)
		{
			::SetLastError(result);
			return false;
		}
	}

	return true;
}

// DllRegisterServer()
// Exported for invocation by COM runtime.
HRESULT __stdcall DllRegisterServer()
{
	// create the transaction for registration operations
	auto txn = CreateTransaction();

	if (!txn)
	{
		return HRESULT_FROM_WIN32(::GetLastError());
	}

	// perform registration
	if (!Register(txn))
	{
		return HRESULT_FROM_WIN32(::GetLastError());
	}

	// commit the transaction
	if (!::CommitTransaction(txn.get()))
	{
		return HRESULT_FROM_WIN32(::GetLastError());
	}

	return S_OK;
}

// DllUnregisterServer()
// Exported for invocation by COM runtime. 
HRESULT __stdcall DllUnregisterServer()
{
	// create the transaction for unregistration operations
	auto txn = CreateTransaction();

	if (!txn)
	{
		return HRESULT_FROM_WIN32(::GetLastError());
	}

	// perform unregistration
	if (!Unregister(txn))
	{
		return HRESULT_FROM_WIN32(::GetLastError());
	}

	// commit the transaction
	if (!::CommitTransaction(txn.get()))
	{
		return HRESULT_FROM_WIN32(::GetLastError());
	}

	return S_OK;
}