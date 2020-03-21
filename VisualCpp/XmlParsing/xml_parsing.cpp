// XmlParsing.cpp
// Powering up Visual C++ with XML parsing via XmlLite.

#include <windows.h>
#include <wrl.h>
#include <ShlObj.h>

#include <xmllite.h>
#pragma comment(lib, "xmllite")

#include <urlmon.h>
#pragma comment(lib, "urlmon")

#include <Shlwapi.h>
#pragma comment(lib, "shlwapi")

#include <cstdio>
#include <string>
#include <vector>
#include <utility>

namespace wrl = Microsoft::WRL;

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

struct com_exception
{
	HRESULT result;

	explicit com_exception(const HRESULT value)
		: result{ value }
	{}
};

using Links = std::vector<std::pair<std::wstring, std::wstring>>;

wrl::ComPtr<IXmlReader> download_reader(const wchar_t* url);

Links get_links(const wrl::ComPtr<IXmlReader>& reader);

template <unsigned Count>
HRESULT find_element(
	const wrl::ComPtr<IXmlReader>& reader,
	const wchar_t(&name)[Count]
	) noexcept;
std::wstring get_value(const wrl::ComPtr<IXmlReader>& reader);

std::wstring get_temp_path();
wrl::ComPtr<IStream> temp_stream(const std::wstring& filename);

void write_html(
	const wrl::ComPtr<IStream>& stream,
	const Links& links
	);

void print_raw_xml(const wrl::ComPtr<IXmlReader>& reader);

void check(const HRESULT result);
void throw_windows_error(const DWORD error = ::GetLastError());

int wmain()
{
	// download the raw XML document
	auto reader = download_reader(L"http://kennykerr.ca/feed/");

	// parse the links from the XML document
	auto links = get_links(reader);

	// generate a temporary file path
	auto filename = get_temp_path();
	filename += L".htm";

	// wrap temporary file in stream
	auto stream = temp_stream(filename);

	// write parsed links to HTML document
	write_html(stream, links);

	// release the stream, ensuring that the stream
	// is flushed to the underlying file prior to rendering
	stream.Reset();

	// open the newly generated HTMl document
	::ShellExecute(
		nullptr,
		nullptr,
		filename.c_str(),
		nullptr,
		nullptr,
		SW_SHOWDEFAULT
	);

	return STATUS_SUCCESS_I;
}

wrl::ComPtr<IXmlReader> 
download_reader(const wchar_t* url)
{
	auto stream = wrl::ComPtr<IStream>{};

	check(::URLOpenBlockingStream(
		nullptr,
		url,
		stream.GetAddressOf(),
		0,
		nullptr
	));

	auto reader = wrl::ComPtr<IXmlReader>{};

	check(::CreateXmlReader(
		__uuidof(reader),
		reinterpret_cast<void**>(reader.GetAddressOf()),
		nullptr
	));

	check(reader->SetInput(stream.Get()));

	return reader;
}

Links 
get_links(const wrl::ComPtr<IXmlReader>& reader)
{
	const wchar_t Item[] = L"item";
	const wchar_t Title[] = L"title";
	const wchar_t Link[] = L"link";

	auto links = Links{};

	while (S_OK == find_element(reader, Item))
	{
		check(find_element(reader, Title));

		auto type = XmlNodeType{};
		check(reader->Read(&type));

		auto title = get_value(reader);

		check(find_element(reader, Link));

		check(reader->Read(&type));

		auto link = get_value(reader);

		links.emplace_back(title, link);
	}

	return links;
}

template <unsigned Count>
HRESULT find_element(
	const wrl::ComPtr<IXmlReader>& reader,
	const wchar_t(&name)[Count]
	) noexcept
{
	auto hr = HRESULT{};
	auto type = XmlNodeType{};

	while (S_OK == (hr = reader->Read(&type)))
	{
		if (XmlNodeType_Element == type)
		{
			const wchar_t* value{};
			auto size = unsigned{};

			hr = reader->GetLocalName(&value, &size);
			if (S_OK != hr)
			{
				break;
			}

			if ((Count - 1) == size && 0 == wcscmp(name, value))
			{
				// found!
				break;
			}
		}
	}

	return hr;
}

std::wstring 
get_value(const wrl::ComPtr<IXmlReader>& reader)
{
	const wchar_t* value{};
	unsigned size{};

	check(reader->GetValue(&value, &size));

	return std::wstring{ value, value + size };
}

std::wstring get_temp_path()
{
	const auto pathSize = ::GetTempPathW(0, nullptr);
	if (!pathSize)
	{
		throw_windows_error();
	}

	const unsigned GuidSize = 38;

	auto path = std::wstring(pathSize + GuidSize - 1, L' ');

	if (!::GetTempPath(pathSize, &path[0]))
	{
		throw_windows_error();
	}

	const auto result = ::SHCreateDirectory(nullptr, path.c_str());

	if (result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS)
	{
		throw_windows_error(result);
	}

	GUID guid;
	check(::CoCreateGuid(&guid));

	// overwrite the 
	::StringFromGUID2(
		guid,
		&path[pathSize - 2],
		GuidSize + 1
	);

	path[pathSize - 2] = L'\\';

	path.resize(pathSize + GuidSize - 3);

	return path;
}

wrl::ComPtr<IStream> 
temp_stream(const std::wstring& filename)
{
	auto stream = wrl::ComPtr<IStream>{};

	check(::SHCreateStreamOnFileEx(
		filename.c_str(),
		STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE,
		FILE_ATTRIBUTE_NORMAL,
		true,
		nullptr,
		stream.GetAddressOf()
	));

	return stream;
}

void write_html(
	const wrl::ComPtr<IStream>& stream,
	const Links& links
	)
{
	auto writer = wrl::ComPtr<IXmlWriter>{};

	check(::CreateXmlWriter(
		__uuidof(writer),
		reinterpret_cast<void**>(writer.GetAddressOf()),
		nullptr
	));

	check(writer->SetProperty(XmlWriterProperty_Indent, true));
	check(writer->SetOutput(stream.Get()));

	check(writer->WriteDocType(L"html", nullptr, nullptr, nullptr));

	check(writer->WriteStartElement(nullptr, L"html", nullptr));

	check(writer->WriteStartElement(nullptr, L"head", nullptr));

	check(writer->WriteStartElement(nullptr, L"meta", nullptr));
	check(writer->WriteAttributeString(nullptr, L"charset", nullptr, L"UTF-8"));
	check(writer->WriteEndElement()); // meta

	check(writer->WriteStartElement(nullptr, L"style", nullptr));
	check(writer->WriteRaw(L"p{font-family:Myriad Pro}"));
	check(writer->WriteEndElement()); // style

	check(writer->WriteEndElement()); // head


	check(writer->WriteStartElement(nullptr, L"body", nullptr));

	for (const auto& e : links)
	{
		check(writer->WriteStartElement(nullptr, L"p", nullptr));

		check(writer->WriteStartElement(nullptr, L"a", nullptr));
		check(writer->WriteAttributeString(nullptr, L"href", nullptr, e.second.c_str()));
		check(writer->WriteChars(e.first.c_str(), e.first.size()));

		check(writer->WriteEndElement()); // a

		check(writer->WriteEndElement()); // p
	}

	check(writer->WriteEndElement()); // body
	check(writer->WriteEndElement()); // html
}

void print_raw_xml(const wrl::ComPtr<IXmlReader>& reader)
{
	auto type = XmlNodeType{};
	auto indent = unsigned{};

	while (S_OK == reader->Read(&type))
	{
		const wchar_t* name{};

		if (XmlNodeType_Element == type)
		{
			check(reader->GetLocalName(&name, nullptr));

			for (unsigned i = 0; i != indent; ++i) printf(" ");

			if (!reader->IsEmptyElement())
			{
				printf("<%S>\n", name);
				++indent;
			}
			else
			{
				printf("<%S />\n", name);
			}
		}
		else if (XmlNodeType_EndElement == type)
		{
			check(reader->GetLocalName(&name, nullptr));

			--indent;

			for (unsigned i = 0; i != indent; ++i) printf(" ");

			printf("</%S>\n", name);
		}
	}
}

void check(const HRESULT result)
{
	if (S_OK != result)
	{
		throw com_exception{ result };
	}
}

void throw_windows_error(const DWORD error)
{
	throw com_exception{ HRESULT_FROM_WIN32(error) };
}