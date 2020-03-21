// client.cpp
// Powering up Visual C++ with Websockets via WinHTTP.

#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp")

#include <string>

#include <wdl/debug.h>
#include <wdl/exception.h>
#include <wdl/unique_handle.h>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

int wmain()
{
	// open the session
	auto session = wdl::winhttp_handle
	{
		::WinHttpOpen(
			nullptr,
			WINHTTP_ACCESS_TYPE_NO_PROXY,
			nullptr,
			nullptr,
			0
			)
	};
	if (!session) throw wdl::windows_exception{};

	// construct a new connection
	auto connection = wdl::winhttp_handle
	{
		::WinHttpConnect(
			session.get(),
			L"localhost",
			8000,
			0
			)
	};
	if (!connection) throw wdl::windows_exception{};

	// initialize a new request
	auto request = wdl::winhttp_handle
	{
		::WinHttpOpenRequest(
			connection.get(),
			nullptr,
			L"/ws",
			nullptr,
			nullptr,
			nullptr,
			0
			)
	};
	if (!request) throw wdl::windows_exception{};

	// set the option necessary to upgrade to a websocket connection
	if (!::WinHttpSetOption(
		request.get(),
		WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,
		nullptr,
		0
	))
	{
		throw wdl::windows_exception{};
	}

	// finally, send the request
	if (!::WinHttpSendRequest(
		request.get(),
		nullptr,
		0,
		nullptr,
		0, 0, 0
	))
	{
		throw wdl::windows_exception{};
	}

	// wait for a response
	if (!::WinHttpReceiveResponse(
		request.get(),
		nullptr
	))
	{
		throw wdl::windows_exception{};
	}

	// confirm that the server is willing to switch protocols to websockets

	auto status = DWORD{};
	auto size = DWORD{ sizeof(status) };

	if (!::WinHttpQueryHeaders(
		request.get(),
		WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX,
		&status,
		&size,
		nullptr
	))
	{
		throw wdl::windows_exception{};
	}

	if (HTTP_STATUS_SWITCH_PROTOCOLS != status)
	{
		printf("Not a websocket server\n");
		return STATUS_FAILURE_I;
	}

	// the upgrade is actually complete here, this is just a way
	// to acquire a handle to the completed connection object
	auto ws = wdl::winhttp_handle
	{
		::WinHttpWebSocketCompleteUpgrade(
			request.get(),
			0
		)
	};

	if (!ws) throw wdl::windows_exception{};

	// read data from the now open websocket connection
	char buffer[1024];
	auto count = DWORD{};
	auto type = WINHTTP_WEB_SOCKET_BUFFER_TYPE{};

	while (ERROR_SUCCESS == ::WinHttpWebSocketReceive(
		ws.get(),
		buffer,
		sizeof(buffer),
		&count,
		&type
	))
	{
		if (WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE == type)
		{
			VERIFY_(ERROR_SUCCESS, ::WinHttpWebSocketClose(
				ws.get(),
				WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS,
				nullptr,
				0
			));

			break;
		}

		if (WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE == type ||
			WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE == type)
		{
			VERIFY_(ERROR_SUCCESS, ::WinHttpWebSocketClose(
				ws.get(),
				WINHTTP_WEB_SOCKET_INVALID_DATA_TYPE_CLOSE_STATUS,
				nullptr,
				0
			));

			break;
		}

		auto message = std::string( buffer, count );

		while (WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE == type)
		{
			auto result = ::WinHttpWebSocketReceive(
				ws.get(),
				buffer,
				sizeof(buffer),
				&count,
				&type
			);

			if (ERROR_SUCCESS != result)
			{
				throw wdl::windows_exception{};
			}

			message.append(buffer, count);
		}

		// print the completed message
		printf("%s\n", message.c_str());
	}
}