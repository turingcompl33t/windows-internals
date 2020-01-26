// Client.cpp
// A minimal Windows sockets client for interacting with server applications.

#define _WINSOCKAPI_ 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <cstdio>
#include <string>
#include <stdexcept>
#include <iostream>

#pragma comment(lib, "ws2_32")

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

constexpr auto BUFFER_SIZE = 512;

void TraceInfo(const char* msg);
void TraceWarning(const char* msg);
void TraceError(const char* msg, DWORD error = ::GetLastError());
void TraceErrorWSA(const char* msg, DWORD error = ::WSAGetLastError());

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		TraceWarning("Error: invalid arguments");
		TraceWarning("Usage: %s <HOST> <PORT>");
		return STATUS_FAILURE_I;
	}

	int     status;
	WSADATA wsdata;
	char    host[128];
	int     port;

	SOCKET             clientSocket;
	struct sockaddr_in server;

	char buffer[BUFFER_SIZE];

	strcpy_s(host, argv[1]);

	try
	{
		port = std::stoi(argv[2]);
	}
	catch (const std::exception& e)
	{
		TraceWarning("Failed to parse arguments");
		printf("[-] %s\n", e.what());
		return STATUS_FAILURE_I;
	}

	if (::WSAStartup(MAKEWORD(2, 2), &wsdata) != 0)
	{
		TraceErrorWSA("WSAStartup failed");
		return STATUS_FAILURE_I;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == clientSocket)
	{
		TraceErrorWSA("socket() failed");
		return STATUS_FAILURE_I;
	}

	server.sin_family = AF_INET;
	server.sin_port   = htons(port);
	inet_pton(AF_INET, host, &server.sin_addr.s_addr);

	if (INADDR_NONE == server.sin_addr.s_addr)
	{
		TraceWarning("Invalid host address specified");
		return STATUS_FAILURE_I;
	}

	if (SOCKET_ERROR == connect(clientSocket, (struct sockaddr*) &server, sizeof(server)))
	{
		TraceErrorWSA("connect() failed");
		return STATUS_FAILURE_I;
	}

	TraceInfo("Connected successfully");

	for (;;)
	{
		std::cout << "> ";
		std::cin.getline(buffer, BUFFER_SIZE);

		status = send(clientSocket, buffer, strnlen_s(buffer, BUFFER_SIZE), 0);
		if (SOCKET_ERROR == status)
		{
			TraceErrorWSA("send() failed");
			break;
		}

		status = recv(clientSocket, buffer, BUFFER_SIZE, 0);
		if (SOCKET_ERROR == status)
		{
			TraceErrorWSA("recv() failed");
			break;
		}

		buffer[BUFFER_SIZE - 1] = '\0';
		std::cout << "[SERVER] " << buffer << std::endl;
	}

	closesocket(clientSocket);
	::WSACleanup();

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