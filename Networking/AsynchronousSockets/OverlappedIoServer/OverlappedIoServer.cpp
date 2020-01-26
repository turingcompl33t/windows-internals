// OverlappedIoServer.cpp
// Simple echo server implementation utilizing overlapped IO.
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
DWORD              g_EventTotal = 0;
WSAEVENT           g_EventArray[WSA_MAXIMUM_WAIT_EVENTS];
SocketInformation* g_SocketArray[WSA_MAXIMUM_WAIT_EVENTS];
CRITICAL_SECTION   g_CriticalSection;

DWORD WINAPI ProcessIO(LPVOID arg);

void TraceInfo(const char* msg);
void TraceWarning(const char* msg);
void TraceError(const char* msg, DWORD error = ::GetLastError());
void TraceErrorWSA(const char* msg, DWORD error = ::WSAGetLastError());

int wmain()
{
	WSADATA     WsaData;
	DWORD       Flags;
	SOCKET      ListenSocket;
	SOCKET      AcceptSocket;
	SOCKADDR_IN Addr;
	DWORD       RecvBytes;
	INT         Status;

	::InitializeCriticalSection(&g_CriticalSection);

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

	Addr.sin_family      = AF_INET;
	Addr.sin_addr.s_addr = htonl(INADDR_ANY);
	Addr.sin_port        = htons(PORT);

	if (SOCKET_ERROR == bind(ListenSocket, (PSOCKADDR)&Addr, sizeof(SOCKADDR)))
	{
		TraceErrorWSA("bind() failed");
		return STATUS_FAILURE_I;
	}

	if (SOCKET_ERROR == listen(ListenSocket, 5))
	{
		TraceErrorWSA("listen() failed");
	}

	g_EventArray[0] = ::WSACreateEvent();
	if (WSA_INVALID_EVENT == g_EventArray[0])
	{
		TraceErrorWSA("WSACreateEvent() failed");
		return STATUS_FAILURE_I;
	}

	if (NULL == ::CreateThread(nullptr, 0, ProcessIO, nullptr, 0, nullptr))
	{
		TraceError("Failed to start worker thread");
		return STATUS_FAILURE_I;
	}

	// initialize the event total index to the first valid index for clients
	g_EventTotal = 1;

	TraceInfo("Started worker thread");
	TraceInfo("Listening...");

	for (;;)
	{
		AcceptSocket = accept(ListenSocket, NULL, NULL);
		if (INVALID_SOCKET == AcceptSocket)
		{
			TraceErrorWSA("accept() failed");
			break;
		}

		::EnterCriticalSection(&g_CriticalSection);

		g_SocketArray[g_EventTotal] = static_cast<SocketInformation*>(
			::HeapAlloc(::GetProcessHeap(), 0, sizeof(SocketInformation))
		);
		if (NULL == g_SocketArray[g_EventTotal])
		{
			TraceError("HeapAlloc() failed");
			break;
		}

		auto& s = g_SocketArray[g_EventTotal];

		s->Socket = AcceptSocket;
		ZeroMemory(&(s->Ov), sizeof(WSAOVERLAPPED));
		s->BytesSend = 0;
		s->BytesRecv = 0;
		s->Data.len = BUFFER_SIZE;
		s->Data.buf = s->Buffer;

		g_EventArray[g_EventTotal] = ::WSACreateEvent();
		if (WSA_INVALID_EVENT == g_EventArray[g_EventTotal])
		{
			TraceErrorWSA("WSACreateEvent failed");
			break;
		}

		// associate the new event with the overlapped structure 
		s->Ov.hEvent = g_EventArray[g_EventTotal];

		// begin an async read operation to begin processing on this new socket
		Flags = 0;
		if (SOCKET_ERROR == ::WSARecv(
			g_SocketArray[g_EventTotal]->Socket,
			&(g_SocketArray[g_EventTotal]->Data),
			1,
			&RecvBytes,
			&Flags,
			&(g_SocketArray[g_EventTotal]->Ov),
			NULL
		))
		{
			auto err = ::WSAGetLastError();
			if (WSA_IO_PENDING != err)
			{
				TraceErrorWSA("WSARecv() failed", err);
				break;
			}
		}

		g_EventTotal++;

		LeaveCriticalSection(&g_CriticalSection);

		// signal the worker thread that a new connection has been accepted
		if (!::WSASetEvent(g_EventArray[0]))
		{
			TraceErrorWSA("WSASetEvent failed");
			break;
		}
	}

	return STATUS_SUCCESS_I;
}

DWORD WINAPI ProcessIO(LPVOID arg)
{
	DWORD              Index;
	DWORD              Flags;
	DWORD              BytesTransferred;
	DWORD              SendBytes;
	DWORD              RecvBytes;
	INT                Status;
	SocketInformation* sd;

	for (;;)
	{
		Index = ::WSAWaitForMultipleEvents(g_EventTotal, g_EventArray, FALSE, INFINITE, FALSE);
		if (WSA_WAIT_FAILED == Index)
		{
			TraceErrorWSA("WSAWaitForMultipleEvents() failed");
			return STATUS_FAILURE_I;
		}

		// determine if this was an event triggered by client accept in main thread
		if (0 == (Index - WSA_WAIT_EVENT_0))
		{
			printf("[+] Accepted new connection\n");
			::WSAResetEvent(g_EventArray[0]);
			continue;
		}

		sd = g_SocketArray[Index - WSA_WAIT_EVENT_0];
		::WSAResetEvent(g_EventArray[Index - WSA_WAIT_EVENT_0]);

		if (!::WSAGetOverlappedResult(
			sd->Socket,
			&(sd->Ov),
			&BytesTransferred,
			FALSE,
			&Flags
		))
		{
			TraceErrorWSA("WSAGetOverlappedResult() failed; closing socket");

			closesocket(sd->Socket);
			::HeapFree(::GetProcessHeap(), 0, sd);
			::WSACloseEvent(g_EventArray[Index - WSA_WAIT_EVENT_0]);

			EnterCriticalSection(&g_CriticalSection);

			if ((Index - WSA_WAIT_EVENT_0) + 1 != g_EventTotal)
			{
				for (int i = Index - WSA_WAIT_EVENT_0; i < g_EventTotal; ++i)
				{
					g_EventArray[i]  = g_EventArray[i + 1];
					g_SocketArray[i] = g_SocketArray[i + 1];
				}
			}

			g_EventTotal--;

			LeaveCriticalSection(&g_CriticalSection);
			continue;
		}

		if (0 == sd->BytesRecv)
		{
			// this is the recv phase of the transaction
			sd->BytesRecv = BytesTransferred;
			sd->BytesSend = 0;
		}
		else
		{
			// this is the send phase of the transaction
			sd->BytesSend += BytesTransferred;
		}

		if (sd->BytesRecv > sd->BytesSend)
		{
			// bytes remain to be sent; post another send operation

			ZeroMemory(&(sd->Ov), sizeof(WSAOVERLAPPED));
			sd->Ov.hEvent = g_EventArray[Index - WSA_WAIT_EVENT_0];
			sd->Data.buf = sd->Buffer + sd->BytesSend;
			sd->Data.len = sd->BytesRecv - sd->BytesSend;

			Status = ::WSASend(
				sd->Socket,
				&(sd->Data),
				1,
				&SendBytes,
				0,
				&(sd->Ov),
				NULL
				);

			if (SOCKET_ERROR == Status)
			{
				auto err = ::WSAGetLastError();
				if (WSA_IO_PENDING != err)
				{
					TraceErrorWSA("WSASend() failed");
					break;
				}
			}
		}
		else
		{
			// the send operation has completed; issue a new read

			sd->BytesRecv = 0;

			Flags = 0;
			ZeroMemory(&(sd->Ov), sizeof(WSAOVERLAPPED));
			sd->Ov.hEvent = g_EventArray[Index - WSA_WAIT_EVENT_0];
			sd->Data.len = BUFFER_SIZE;
			sd->Data.buf = sd->Buffer;

			Status = ::WSARecv(
				sd->Socket,
				&(sd->Data),
				1,
				&RecvBytes,
				&Flags,
				&(sd->Ov),
				NULL
				);

			if (SOCKET_ERROR == Status)
			{
				auto err = ::WSAGetLastError();
				if (WSA_IO_PENDING != err)
				{
					TraceErrorWSA("WSARecv() failed", err);
				}
			}
		}
	}

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
