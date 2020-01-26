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

constexpr auto PORT        = 8000;
constexpr auto BUFFER_SIZE = 512;

struct IoOperationData
{
	OVERLAPPED Ov;
	WSABUF     Data;
	CHAR       Buffer[BUFFER_SIZE];
	DWORD      BytesSend;
	DWORD      BytesRecv;
};

struct HandleData
{
	SOCKET Socket;
};

DWORD WINAPI WorkerThread(LPVOID completionPortId);

void TraceInfo(const char* msg);
void TraceWarning(const char* msg);
void TraceError(const char* msg, DWORD error = ::GetLastError());
void TraceErrorWSA(const char* msg, DWORD error = ::WSAGetLastError());

int wmain()
{
	SOCKADDR_IN Addr;
	SOCKET      ListenSocket;
	HANDLE      hThread;
	SOCKET      AcceptSocket;
	HANDLE      hCompletionPort;
	SYSTEM_INFO Info;

	DWORD   RecvBytes;
	DWORD   Flags;
	DWORD   ThreadId;
	WSADATA WsaData;
	
	HandleData*      hd;
	IoOperationData* od;
	DWORD            status;

	if ((status = ::WSAStartup((2, 2), &WsaData)) != 0)
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
	::GetSystemInfo(&Info);
	for (unsigned i = 0; i < Info.dwNumberOfProcessors*2; ++i)
	{
		hThread = ::CreateThread(nullptr, 0, WorkerThread, hCompletionPort, 0, &ThreadId);
		if (NULL == hThread)
		{
			TraceError("CreateThread failed\n");
			return STATUS_FAILURE_I;
		}
		else
		{
			printf("[+] Started thread %u\n", ThreadId);
		}

		// TODO: why?
		::CloseHandle(hThread);
	}

	ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == ListenSocket)
	{
		TraceError("WSASocket() failed");
		return STATUS_FAILURE_I;
	}
	
	Addr.sin_family      = AF_INET;
	Addr.sin_addr.s_addr = htonl(INADDR_ANY);
	Addr.sin_port        = htons(PORT);

	if (SOCKET_ERROR == bind(ListenSocket, (PSOCKADDR)&Addr, sizeof(Addr)))
	{
		TraceErrorWSA("bind() failed");
		return STATUS_FAILURE_I;
	}

	if (SOCKET_ERROR == listen(ListenSocket, 5))
	{
		TraceErrorWSA("listen() failed");
		return STATUS_FAILURE_I;
	}

	TraceInfo("Listening...");

	for (;;)
	{
		AcceptSocket = WSAAccept(ListenSocket, NULL, NULL, NULL, 0);
		if (SOCKET_ERROR == AcceptSocket)
		{
			TraceErrorWSA("WSAAccept failed");
			return STATUS_FAILURE_I;
		}

		TraceInfo("Accepted new connection");

		hd = static_cast<HandleData*>(::HeapAlloc(::GetProcessHeap(), 0, sizeof(HandleData)));
		if (nullptr == hd)
		{
			TraceError("HeapAlloc() failed");
			return STATUS_FAILURE_I;
		}

		hd->Socket = AcceptSocket;

		if (NULL == ::CreateIoCompletionPort(
			reinterpret_cast<HANDLE>(AcceptSocket),
			hCompletionPort,
			reinterpret_cast<ULONG_PTR>(hd),
			0
		))
		{
			TraceError("CreateIoCompletionPort() failed");
			return STATUS_FAILURE_I;
		}

		od = static_cast<IoOperationData*>(
			::HeapAlloc(::GetProcessHeap(), 0, sizeof(IoOperationData))
			);
		if (nullptr == od)
		{
			TraceError("HeapAlloc() failed");
			return STATUS_FAILURE_I;
		}

		ZeroMemory(&(od->Ov), sizeof(WSAOVERLAPPED));
		od->BytesRecv = 0;
		od->BytesSend = 0;
		od->Data.len = BUFFER_SIZE;
		od->Data.buf = od->Buffer;

		Flags = 0;

		// initiate an asynchronous recv on the newly accepted socket
		if (SOCKET_ERROR == ::WSARecv(
			AcceptSocket,
			&(od->Data),
			1,
			&RecvBytes,
			&Flags,
			&(od->Ov),
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
	
	DWORD BytesTransferred;

	DWORD SendBytes;
	DWORD RecvBytes;
	DWORD Flags;

	HandleData*      hd;
	IoOperationData* od;

	for (;;)
	{
		if (0 == ::GetQueuedCompletionStatus(
			hCompletionPort,
			&BytesTransferred,
			reinterpret_cast<PULONG_PTR>(&hd),
			reinterpret_cast<LPOVERLAPPED*>(&od),
			INFINITE
		))
		{
			TraceError("GetQueuedCompletionStatus() failed");
			return STATUS_FAILURE_I;
		}

		// check for error
		if (0 == BytesTransferred)
		{
			TraceWarning("Error occurred on socket; closing...");
			if (SOCKET_ERROR == closesocket(hd->Socket))
			{
				TraceErrorWSA("closesocket() failed");
				return STATUS_FAILURE_I;
			}

			::HeapFree(::GetProcessHeap(), 0, hd);
			::HeapFree(::GetProcessHeap(), 0, od);
			continue;
		}

		// no error occurred on the operation, process the completed operation

		if (0 == od->BytesRecv)
		{
			// a recv operation just completed
			od->BytesRecv = BytesTransferred;
			od->BytesSend = 0;
		}
		else
		{
			// a send operation just completed
			od->BytesSend = BytesTransferred;
		}

		if (od->BytesRecv > od->BytesSend)
		{
			// there remain bytes that were recved that have not yet been echoed back to client

			ZeroMemory(&(od->Ov), sizeof(WSAOVERLAPPED));
			// compute the location within the buffer where the send should begin
			od->Data.buf = od->Buffer + od->BytesSend;
			// compute the size of the remaining data
			od->Data.len = od->BytesRecv - od->BytesSend;

			// issue the send operation
			if (SOCKET_ERROR == ::WSASend(
				hd->Socket,
				&(od->Data),
				1,
				&SendBytes,
				0,
				&(od->Ov),
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

			od->BytesRecv = 0;

			Flags = 0;
			ZeroMemory(&(od->Ov), sizeof(OVERLAPPED));
			od->Data.len = BUFFER_SIZE;
			od->Data.buf = od->Buffer;

			if (SOCKET_ERROR == ::WSARecv(
				hd->Socket,
				&(od->Data),
				1,
				&RecvBytes,
				&Flags,
				&(od->Ov),
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