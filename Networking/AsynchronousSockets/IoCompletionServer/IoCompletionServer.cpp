// IoCompletionServer.cpp
// Simple echo server implementation utilizing Windows IO completion port.

#define _WINSOCKAPI_ 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>
#include <cstdio>

#pragma comment(lib, "ws2_32")

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

// server listen port
constexpr auto PORT        = 8000;
// recv buffer size
constexpr auto BUFFER_SIZE = 512;

struct IoOperationData
{
	OVERLAPPED ov;
	WSABUF data;
	CHAR buffer[BUFFER_SIZE];
	DWORD bytesSend;
	DWORD bytesRecv;
};

struct HandleData
{
	SOCKET socket;
};

DWORD WINAPI WorkerThread(LPVOID completionPortId);

void TraceInfo(const char* msg);
void TraceWarning(const char* msg);
void TraceError(const char* msg, DWORD error = ::GetLastError());
void TraceErrorWSA(const char* msg, DWORD error = ::WSAGetLastError());

int wmain()
{
	SOCKADDR_IN addr;
	SOCKET listenSocket;
	HANDLE hThread;
	SOCKET acceptSocket;
	HANDLE hCompletionPort;
	SYSTEM_INFO info;
	
	HandleData* handleData;
	IoOperationData* operationData;

	DWORD recvBytes;
	DWORD flags;
	DWORD threadId;
	WSADATA wsaData;
	DWORD status;

	if ((status = ::WSAStartup((2, 2), &wsaData)) != 0)
	{
		TraceError("WSAStartup failed");
		return STATUS_FAILURE_I;
	}

	// create a new IO completion port
	// here we specify that the port should allow the number 
	// of concurrent threads to max out at the number of processors
	// on the current system
	hCompletionPort = ::CreateIoCompletionPort(
		INVALID_HANDLE_VALUE,
		nullptr,
		0,
		0   
		);
	if (NULL == hCompletionPort)
	{
		TraceError("CreateIoCompletionPort() failed");
		return STATUS_FAILURE_I;
	}
	
	// determine the number of processors on system and create threads accordingly
	::GetSystemInfo(&info);
	for (unsigned i = 0; i < info.dwNumberOfProcessors*2; ++i)
	{
		hThread = ::CreateThread(nullptr, 0, WorkerThread, hCompletionPort, 0, &threadId);
		if (NULL == hThread)
		{
			TraceError("CreateThread failed\n");
			return STATUS_FAILURE_I;
		}
		else
		{
			printf("[+] Started thread %u\n", threadId);
		}

		// TODO: why?
		::CloseHandle(hThread);
	}

	listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == listenSocket)
	{
		TraceError("WSASocket() failed");
		return STATUS_FAILURE_I;
	}
	
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port        = htons(PORT);

	if (SOCKET_ERROR == bind(listenSocket, (PSOCKADDR)&addr, sizeof(addr)))
	{
		TraceErrorWSA("bind() failed");
		return STATUS_FAILURE_I;
	}

	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		TraceErrorWSA("listen() failed");
		return STATUS_FAILURE_I;
	}

	TraceInfo("Listening...");

	for (;;)
	{
		acceptSocket = WSAAccept(listenSocket, NULL, NULL, NULL, 0);
		if (SOCKET_ERROR == acceptSocket)
		{
			TraceErrorWSA("WSAAccept failed");
			return STATUS_FAILURE_I;
		}

		TraceInfo("Accepted new connection");

		handleData = static_cast<HandleData*>(::HeapAlloc(::GetProcessHeap(), 0, sizeof(HandleData)));
		if (nullptr == handleData)
		{
			TraceError("HeapAlloc() failed");
			return STATUS_FAILURE_I;
		}

		handleData->socket = acceptSocket;

		if (NULL == ::CreateIoCompletionPort(
			reinterpret_cast<HANDLE>(acceptSocket),
			hCompletionPort,
			reinterpret_cast<ULONG_PTR>(handleData),
			0
		))
		{
			TraceError("CreateIoCompletionPort() failed");
			return STATUS_FAILURE_I;
		}

		operationData = static_cast<IoOperationData*>(
			::HeapAlloc(::GetProcessHeap(), 0, sizeof(IoOperationData))
			);
		if (nullptr == operationData)
		{
			TraceError("HeapAlloc() failed");
			return STATUS_FAILURE_I;
		}

		ZeroMemory(&(operationData->ov), sizeof(OVERLAPPED));
		operationData->bytesRecv = 0;
		operationData->bytesSend = 0;
		operationData->data.len = BUFFER_SIZE;
		operationData->data.buf = operationData->buffer;

		flags = 0;

		// initiate an asynchronous recv on the newly accepted socket
		if (SOCKET_ERROR == ::WSARecv(
			acceptSocket,
			&(operationData->data),
			1,
			&recvBytes,
			&flags,
			&(operationData->ov),
			NULL
		))
		{
			auto err = ::WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				TraceErrorWSA("WSARecv() failed", err);
				return STATUS_FAILURE_I;
			}
		}
	}
}

DWORD WINAPI WorkerThread(LPVOID completionPortId)
{
	HANDLE hCompletionPort = static_cast<HANDLE>(completionPortId);
	DWORD bytesTransferred;
	HandleData* handleData;
	IoOperationData* operationData;
	DWORD sendBytes;
	DWORD recvBytes;
	DWORD flags;

	for (;;)
	{
		if (0 == ::GetQueuedCompletionStatus(
			hCompletionPort,
			&bytesTransferred,
			reinterpret_cast<PULONG_PTR>(&handleData),
			reinterpret_cast<LPOVERLAPPED*>(&operationData),
			INFINITE
		))
		{
			TraceError("GetQueuedCompletionStatus() failed");
			return STATUS_FAILURE_I;
		}

		// check for error
		if (0 == bytesTransferred)
		{
			TraceWarning("Error occurred on socket; closing...");
			if (SOCKET_ERROR == closesocket(handleData->socket))
			{
				TraceErrorWSA("closesocket() failed");
				return STATUS_FAILURE_I;
			}

			::HeapFree(::GetProcessHeap(), 0, handleData);
			::HeapFree(::GetProcessHeap(), 0, operationData);
			continue;
		}

		// no error occurred on the operation, process the completed operation

		if (0 == operationData->bytesRecv)
		{
			// a recv operation just completed
			operationData->bytesRecv = bytesTransferred;
			operationData->bytesSend = 0;
		}
		else
		{
			// a send operation just completed
			operationData->bytesSend = bytesTransferred;
		}

		if (operationData->bytesRecv > operationData->bytesSend)
		{
			// there remain bytes that were recved that have not yet been echoed back to client

			ZeroMemory(&(operationData->ov), sizeof(OVERLAPPED));
			// compute the location within the buffer where the send should begin
			operationData->data.buf = operationData->buffer + operationData->bytesSend;
			// compute the size of the remaining data
			operationData->data.len = operationData->bytesRecv - operationData->bytesSend;

			// issue the send operation
			if (SOCKET_ERROR == ::WSASend(
				handleData->socket,
				&(operationData->data),
				1,
				&sendBytes,
				0,
				&(operationData->ov),
				NULL
			))
			{
				TraceErrorWSA("WSASend() failed");
				return STATUS_FAILURE_I;
			}
		}
		else
		{
			// a send operation has completed successfully; queue another recv

			operationData->bytesRecv = 0;

			flags = 0;
			ZeroMemory(&(operationData->ov), sizeof(OVERLAPPED));
			operationData->data.len = BUFFER_SIZE;
			operationData->data.buf = operationData->buffer;

			if (SOCKET_ERROR == ::WSARecv(
				handleData->socket,
				&(operationData->data),
				1,
				&recvBytes,
				&flags,
				&(operationData->ov),
				NULL
			))
			{
				auto err = ::WSAGetLastError();
				if (err != ERROR_IO_PENDING)
				{
					TraceErrorWSA("WSARecv() failed");
					return STATUS_FAILURE_I;
				}
			}
		}
	}

	// not reached
	return STATUS_SUCCESS_I;
}

void TraceInfo(const char* msg)
{
	printf("[+] %s\n", msg);
}

void TraceWarning(const char* msg)
{
	printf("[-] %s\n", msg);
}

void TraceError(const char* msg, DWORD error)
{
	printf("[!] %s (%u)\n", msg, error);
}

void TraceErrorWSA(const char* msg, DWORD error)
{
	printf("[!] %s (%u)\n", msg, error);
}