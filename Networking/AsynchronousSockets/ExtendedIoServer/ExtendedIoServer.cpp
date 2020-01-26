// ExtendedIoServer.cpp
// Simple echo server implementation utilizing extended IO with callback routines.
//
// NOTE: this example leaves much to be desired in terms of
// proper error-handling and cleanup; while these considerations
// are always important, they are far from the focus
// of this example.

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

struct SocketInformation
{
	OVERLAPPED Ov;
	SOCKET     Socket;
	CHAR       Buffer[BUFFER_SIZE];
	WSABUF     Data;
	DWORD      BytesSend;
	DWORD      BytesRecv;
};

// global state
SOCKET g_AcceptSocket;

DWORD WINAPI WorkerThread(LPVOID arg);
void CALLBACK OnIoComplete(
	DWORD Error,
	DWORD BytesTransferred,
	LPWSAOVERLAPPED Ov,
	DWORD InFlags
	);

void TraceInfo(const char* msg);
void TraceWarning(const char* msg);
void TraceError(const char* msg, DWORD error = ::GetLastError());
void TraceErrorWSA(const char* msg, DWORD error = ::WSAGetLastError());

int wmain()
{
	WSADATA     WsaData;
	SOCKET      ListenSocket;
	SOCKADDR_IN Addr;

	HANDLE   hWorkerThread;
	DWORD    ThreadId;
	WSAEVENT hAcceptEvent;
	INT      Status;

	if ((Status = ::WSAStartup(MAKEWORD(2, 2), &WsaData)) != 0)
	{
		TraceErrorWSA("WSAStartup failed");
		return STATUS_FAILURE_I;
	}

	ListenSocket = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == ListenSocket)
	{
		TraceErrorWSA("WSASocket() failed");
		return STATUS_FAILURE_I;
	}

	Addr.sin_family = AF_INET;
	Addr.sin_addr.s_addr = htonl(INADDR_ANY);
	Addr.sin_port = htons(PORT);

	if (SOCKET_ERROR == bind(ListenSocket, (PSOCKADDR)&Addr, sizeof(SOCKADDR)))
	{
		TraceErrorWSA("bind() failed");
		return STATUS_FAILURE_I;
	}

	if (SOCKET_ERROR == listen(ListenSocket, 5))
	{
		TraceErrorWSA("listen() failed");
	}

	hAcceptEvent = ::WSACreateEvent();
	if (WSA_INVALID_EVENT == hAcceptEvent)
	{
		TraceErrorWSA("WSACreateEvent failed");
		return STATUS_FAILURE_I;
	}

	hWorkerThread = ::CreateThread(
		nullptr,
		0,
		WorkerThread,
		(LPVOID)hAcceptEvent,
		0,
		&ThreadId
		);

	if (NULL == hWorkerThread)
	{
		TraceError("CreateThread failed");
		return STATUS_FAILURE_I;
	}

	printf("[+] Started worker thread %u\n", ThreadId);
	TraceInfo("Listening...");

	for (;;)
	{
		g_AcceptSocket = accept(ListenSocket, NULL, NULL);

		if (!::WSASetEvent(hAcceptEvent))
		{
			TraceErrorWSA("WSASetEvent failed");
			break;
		}
	}

	return STATUS_SUCCESS_I;
}

DWORD WINAPI WorkerThread(LPVOID arg)
{
	INT                Status;
	DWORD              Flags;
	SocketInformation* SocketData;
	WSAEVENT           EventArray[1];
	DWORD              Index;
	DWORD              RecvBytes;

	EventArray[0] = static_cast<WSAEVENT>(arg);

	for (;;)
	{
		for (;;)
		{
			Index = ::WSAWaitForMultipleEvents(1, EventArray, FALSE, INFINITE, TRUE);
			if (WSA_WAIT_FAILED == Index)
			{
				TraceErrorWSA("WSAWaitForMultipleObjects() reported wait failure");
				return STATUS_FAILURE_I;
			}

			if (WAIT_IO_COMPLETION != Index)
			{
				// signaled by the main thread; accept() call has completed
				break;
			}
		}

		::WSAResetEvent(EventArray[Index - WSA_WAIT_EVENT_0]);

		printf("[+] Accepted new connection\n");

		SocketData = static_cast<SocketInformation*>(
			::HeapAlloc(::GetProcessHeap(), 0, sizeof(SocketData))
			);
		if (NULL == SocketData)
		{
			TraceError("HeapAlloc() failed");
			return STATUS_FAILURE_I;
		}

		SocketData->Socket = g_AcceptSocket;
		ZeroMemory(&(SocketData->Ov), sizeof(WSAOVERLAPPED));
		SocketData->BytesSend = 0;
		SocketData->BytesRecv = 0;
		SocketData->Data.len = BUFFER_SIZE;
		SocketData->Data.buf = SocketData->Buffer;

		Flags = 0;
		
		// issue the async read on the socket with completion routine specified
		Status = ::WSARecv(
			SocketData->Socket,
			&(SocketData->Data),
			1,
			&RecvBytes,
			&Flags,
			&(SocketData->Ov),
			OnIoComplete
			);

		if (SOCKET_ERROR == Status)
		{
			auto err = ::WSAGetLastError();
			if (WSA_IO_PENDING != err)
			{
				TraceErrorWSA("WSARecv() failed", err);
				break;
			}
		}
	}

	return STATUS_SUCCESS_I;
}

void CALLBACK OnIoComplete(
	DWORD Error,
	DWORD BytesTransferred,
	LPWSAOVERLAPPED Ov,
	DWORD InFlags
	)
{
	DWORD Status;
	DWORD SendBytes;
	DWORD RecvBytes;
	DWORD Flags;

	// reference the overlapped structure as the socket information
	// structure which contains it 
	SocketInformation* sd = reinterpret_cast<SocketInformation*>(Ov);

	if (Error != 0 || 0 == BytesTransferred)
	{
		TraceError("IO operation completed with error", Error);
		TraceError("Closing socket", 0);

		closesocket(sd->Socket);
		::HeapFree(::GetProcessHeap(), 0, sd);
		return;
	}

	if (0 == sd->BytesRecv)
	{
		// this is the recv phase of the communication transaction
		sd->BytesRecv = BytesTransferred;
		sd->BytesSend = 0;
	}
	else
	{
		// this is the send phase of the communication transaction
		sd->BytesSend += BytesTransferred;
	}

	if (sd->BytesRecv > sd->BytesSend)
	{
		// there are bytes that were recved that remain to be sent to client

		ZeroMemory(&(sd->Ov), sizeof(WSAOVERLAPPED));
		sd->Data.buf = sd->Buffer + sd->BytesSend;
		sd->Data.len = sd->BytesRecv - sd->BytesSend;

		Status = ::WSASend(
			sd->Socket,
			&(sd->Data),
			1,
			&SendBytes,
			0,
			&(sd->Ov),
			OnIoComplete
		);

		if (SOCKET_ERROR == Status)
		{
			auto err = ::WSAGetLastError();
			if (WSA_IO_PENDING != err)
			{
				TraceErrorWSA("WSASend() failed", err);
				return;
			}
		}
	}
	else
	{
		// the send phase of the transaction has completed; issue another read

		sd->BytesRecv = 0;

		Flags = 0;
		ZeroMemory(&(sd->Ov), sizeof(WSAOVERLAPPED));
		sd->Data.len = BUFFER_SIZE;
		sd->Data.buf = sd->Buffer;

		Status = ::WSARecv(
			sd->Socket,
			&(sd->Data),
			1,
			&RecvBytes,
			&Flags,
			&(sd->Ov),
			OnIoComplete
			);
		if (SOCKET_ERROR == Status)
		{
			auto err = ::WSAGetLastError();
			if (WSA_IO_PENDING == err)
			{
				TraceErrorWSA("WSARecv() failed", err);
				return;
			}
		}
	}
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

