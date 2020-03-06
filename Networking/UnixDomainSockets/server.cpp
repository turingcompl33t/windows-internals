// server.cpp
// Simple server application utilizing UNIX domain sockets.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 server.cpp /link /DEBUG

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <afunix.h>
#include <stdio.h>
#include <io.h>

#include <atomic>
#include <iostream>
#include <string_view>

#pragma comment(lib, "ws2_32")

constexpr auto SOCKADDR_UN_PATH = "foo.socket";

constexpr auto BUFFER_SIZE = 256;

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

using SOCKADDR_UN = struct sockaddr_un;

std::atomic_bool g_shutdown_flag{ false };

void trace_error_wsa(
    const std::string_view msg, 
    const unsigned long code = ::WSAGetLastError()
    )
{
    std::cout << "[-] " << msg << ';';
    std::cout << " Error Code: " << code << '\n';
}

void trace_error_win32(
    const std::string_view msg,
    const unsigned long code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg << ';';
    std::cout << " Error Code: " << code << '\n';
}

void trace_info(const std::string msg)
{
    std::cout << "[+] " << msg << '\n';
}

SOCKET server_listen(const char* name)
{
    auto sock = ::WSASocketW(AF_UNIX, SOCK_STREAM, 0, nullptr, 0, 0);
    if (INVALID_SOCKET == sock)
    {
        trace_error_wsa("WSASocket() failed");
        return INVALID_SOCKET;
    }

    _unlink(name);

    SOCKADDR_UN addr{};
    addr.sun_family = AF_UNIX;
    strncpy_s(addr.sun_path, name, UNIX_PATH_MAX);

    if (SOCKET_ERROR == ::bind(sock, (sockaddr*)&addr, sizeof(addr)))
    {
        trace_error_wsa("bind() failed");
        return INVALID_SOCKET;
    }

    if (SOCKET_ERROR == ::listen(sock, 1))
    {
        trace_error_wsa("listen() failed");
        return INVALID_SOCKET;
    }

    return sock;
}

SOCKET server_accept(SOCKET listen_sock)
{
    SOCKADDR client_addr{};
    int client_addrlen = sizeof(SOCKADDR);

    auto client_sock = ::WSAAccept(
        listen_sock, 
        &client_addr, 
        &client_addrlen, 
        nullptr, 
        0
        );

    if (INVALID_SOCKET  == client_sock)
    {
        trace_error_wsa("WSAAccept() failed");
    }

    return client_sock;
}

void handle_client(SOCKET sock)
{
    char buffer[BUFFER_SIZE];
    ZeroMemory(buffer, BUFFER_SIZE);
    ULONG buffer_size = BUFFER_SIZE;

    WSABUF wsa_buf;
    wsa_buf.buf = buffer;
    wsa_buf.len = buffer_size;

    unsigned long bytes_recv = 0;
    unsigned long bytes_send = 0;

    for (;;)
    {
        // recv the message from client
        if (SOCKET_ERROR == ::WSARecv(
            sock, 
            &wsa_buf, 
            1, 
            &bytes_recv, 
            0, 
            nullptr, 
            nullptr
        ))
        {
            trace_error_wsa("WSARecv() failed");
            break;
        }

        // echo the response
        if (SOCKET_ERROR == ::WSASend(
            sock, 
            &wsa_buf, 
            1, 
            &bytes_send, 
            0, 
            nullptr, 
            nullptr
        ))
        {
            trace_error_wsa("WSDASend() failed");
            break;
        }

        ZeroMemory(buffer, BUFFER_SIZE);
    }
}

BOOL __stdcall console_ctrl_handler(unsigned long code)
{
    UNREFERENCED_PARAMETER(code);

    g_shutdown_flag.exchange(true);
    return TRUE;
}

int main()
{
    WSADATA wsa_data;

    if (::WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        trace_error_wsa("WSAStartup() failed");
        return STATUS_FAILURE_I;
    }

    if (!::SetConsoleCtrlHandler(console_ctrl_handler, TRUE))
    {
        trace_error_win32("SetConsoleCtrlHandler() failed");
        return STATUS_FAILURE_I;
    }

    auto listen_sock = server_listen(SOCKADDR_UN_PATH);
    if (INVALID_SOCKET == listen_sock)
    {
        return STATUS_FAILURE_I;
    }

    trace_info("Listening for client connections");

    while (!g_shutdown_flag)
    {
        auto client_sock = server_accept(listen_sock);
        if (INVALID_SOCKET == client_sock)
        {
            break;
        }

        trace_info("Accepted new client connection");

        handle_client(client_sock);

        ::closesocket(client_sock);
    }

    ::closesocket(listen_sock);
    ::WSACleanup();

    return STATUS_SUCCESS_I;
}