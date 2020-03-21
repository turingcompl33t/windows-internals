// file_download.cpp
// Powering up Visual C++ with simple internet file downloads.
//
// Usage:
//	file_download.exe <URL> <MODE>
//
// Mode Options:
//	-f -> download to file
//	-c -> download to browser cache
//	-s -> download to stream

#include <windows.h>
#include <urlmon.h>
#include <wrl.h>

#include <cstdio>

#pragma comment(lib, "urlmon")

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr auto FILE_PATH = L"C:\\Dev\\PowerfulVC\\Output\\page.html";

using namespace Microsoft::WRL;

enum class DownloadMode
{
	File,
	Cache,
	Stream,
	Invalid
};

bool DownloadToFile(
	const wchar_t* url,
	const wchar_t* file);
bool DownloadToBrowserCache(const wchar_t* url);
bool DownloadToStream(const wchar_t* url);

COORD GetPosition(HANDLE console);
void ShowCursor(HANDLE console, const bool visible = true);

DownloadMode ParseDownloadMode(const wchar_t* mode_str);
void PrintUsage(const wchar_t* prog);

struct StreamInfo : STATSTG
{
	StreamInfo() : STATSTG{}
	{}

	~StreamInfo()
	{
		if (pwcsName)
		{
			::CoTaskMemFree(pwcsName);
		}
	}
};

struct ProgressCallback :
	RuntimeClass<RuntimeClassFlags<ClassicCom>,
				 IBindStatusCallback>
{
	HANDLE m_console;
	COORD m_position;

	ProgressCallback(HANDLE console, COORD position)
		: m_console(console), m_position(position)
	{}

	HRESULT __stdcall OnProgress(
		ULONG progress,
		ULONG progressMax,
		ULONG statusCode,
		LPCWSTR statusText
	) override
	{
		if (0 < progress && progress <= progressMax)
		{
			float percentF = progress * 100.0f / progressMax;
			unsigned percentU = min(100U, static_cast<unsigned>(percentF));

			::SetConsoleCursorPosition(m_console, m_position);

			printf("%3d%%\n", percentU);
		}

		// can use the below to get fine-grained event updates
		// from the underlying IE request logic 
		//const auto statusEnum = static_cast<BINDSTATUS>(statusCode);

		//printf("%2d> %S\n(%d/%d)\n\n",
		//	statusEnum,
		//	statusText,
		//	progress,
		//	progressMax);

		return S_OK;
	}

	HRESULT __stdcall OnStartBinding(DWORD, IBinding*) override
	{
		return E_NOTIMPL;
	}

	HRESULT __stdcall GetPriority(LONG*) override
	{
		return E_NOTIMPL;
	}

	HRESULT __stdcall OnLowResource(DWORD) override
	{
		return E_NOTIMPL;
	}

	HRESULT __stdcall OnStopBinding(HRESULT, LPCWSTR) override
	{
		return E_NOTIMPL;
	}

	HRESULT __stdcall GetBindInfo(DWORD*, BINDINFO*) override
	{
		return E_NOTIMPL;
	}

	HRESULT __stdcall OnDataAvailable(
		DWORD,
		DWORD,
		FORMATETC*,
		STGMEDIUM*
	) override
	{
		return E_NOTIMPL;
	}

	HRESULT __stdcall OnObjectAvailable(REFIID, IUnknown*) override
	{
		return E_NOTIMPL;
	}
};

int wmain(int argc, wchar_t* argv[])
{
	if (argc != 3)
	{
		PrintUsage(argv[0]);
		return STATUS_FAILURE_I;
	}

	auto mode = ParseDownloadMode(argv[2]);
	if (mode == DownloadMode::Invalid)
	{
		printf("[-] Invalid download mode specified\n");
		return STATUS_FAILURE_I;
	}

	bool res{};

	switch (mode)
	{
	case DownloadMode::File:
		res = DownloadToFile(argv[1], FILE_PATH);
		break;
	case DownloadMode::Cache:
		res = DownloadToBrowserCache(argv[1]);
		break;
	case DownloadMode::Stream:
		res = DownloadToStream(argv[1]);
		break;
	}

	if (!res)
	{
		printf("[-] Download failed\n");
	}
	else
	{
		printf("[+] Download succeeded\n");
	}

	return STATUS_SUCCESS_I;
}

bool DownloadToFile(
	const wchar_t* url, 
	const wchar_t* file
	)
{
	HRESULT result; 

	auto console = ::GetStdHandle(STD_OUTPUT_HANDLE);

	ShowCursor(console, false);

	auto callback = ProgressCallback
	{
		console,
		GetPosition(console)
	};

	result = ::URLDownloadToFileW(
		nullptr,
		url,
		file,
		0,
		&callback
	);

	ShowCursor(console, true);

	return result == S_OK;
}

bool DownloadToBrowserCache(const wchar_t* url)
{
	HRESULT result;
	wchar_t filename[MAX_PATH];

	auto console = ::GetStdHandle(STD_OUTPUT_HANDLE);
	
	ShowCursor(console, false);

	auto callback = ProgressCallback
	{
		console,
		GetPosition(console)
	};

	result = ::URLDownloadToCacheFileW(
		nullptr,
		url,
		filename,
		_countof(filename),
		0,
		&callback
	);

	ShowCursor(console, true);

	return result == S_OK;
}

bool DownloadToStream(const wchar_t* url)
{
	HRESULT result;
	auto stream  = ComPtr<IStream>{};
	auto console = ::GetStdHandle(STD_OUTPUT_HANDLE);

	ShowCursor(console, false);

	auto callback = ProgressCallback
	{
		console,
		GetPosition(console)
	};

	result = ::URLOpenBlockingStreamW(
		nullptr,
		url,
		stream.GetAddressOf(),
		0,
		&callback
	);

	auto info = StreamInfo{};
	stream->Stat(&info, STATFLAG_DEFAULT);
	
	printf("Download Size: %llu\n", info.cbSize.QuadPart);

	return result == S_OK;
}

COORD GetPosition(HANDLE console)
{
	CONSOLE_SCREEN_BUFFER_INFO info;

	::GetConsoleScreenBufferInfo(console, &info);

	return info.dwCursorPosition;
}

void ShowCursor(HANDLE console, const bool visible)
{
	CONSOLE_CURSOR_INFO info;

	::GetConsoleCursorInfo(console, &info);
	info.bVisible = visible;
	::SetConsoleCursorInfo(console, &info);
}

DownloadMode ParseDownloadMode(const wchar_t* mode_str)
{
	if (0 == wcscmp(mode_str, L"-f"))
	{
		return DownloadMode::File;
	}
	else if (0 == wcscmp(mode_str, L"-c"))
	{
		return DownloadMode::Cache;
	}
	else if (0 == wcscmp(mode_str, L"-s"))
	{
		return DownloadMode::Stream;
	}
	else
	{
		return DownloadMode::Invalid;
	}
}

void PrintUsage(const wchar_t* prog)
{
	printf("[-] Error: invalid arguments\n");
	printf("[-] Usage: %ws <URL> <MODE>\n", prog);
	printf("\t-f -> download to file\n");
	printf("\t-c -> download to cache\n");
	printf("\t-s -> download to stream\n");
}