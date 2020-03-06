// client.cpp
// Simple client application utilizing UNIX domain sockets.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 client.cpp

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <afunix.h>

#include <string>
#include <sstream>
#include <iostream>
#include <string_view>

#pragma comment(lib, "ws2_32")

constexpr auto SOCKADDR_UN_PATH = "foo.socket";

constexpr auto MAX_MESSAGE_SIZE = 256;

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

using SOCKADDR_UN = struct sockaddr_un;

void trace_error_wsa(
    const std::string_view msg, 
    const unsigned long code = ::WSAGetLastError()
    )
{
    std::cout << "[-] " << msg << ';';
    std::cout << "Error Code: " << code << '\n';
}

void trace_error_win32(
    const std::string_view msg,
    const unsigned long code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg << ';';
    std::cout << "Error Code: " << code << '\n';
}

void trace_info(const std::string msg)
{
    std::cout << "[+] " << msg << '\n';
}

SOCKET client_connect(const char* name)
{
    auto sock = ::WSASocketW(AF_UNIX, SOCK_STREAM, 0, nullptr, 0, 0);
    if (INVALID_SOCKET == sock)
    {
        trace_error_wsa("WSASocket() failed");
        return INVALID_SOCKET;
    }

    SOCKADDR_UN addr{};
    addr.sun_family = AF_UNIX;
    strncpy_s(addr.sun_path, name, UNIX_PATH_MAX);

    if (SOCKET_ERROR == ::WSAConnect(
        sock, 
        (sockaddr*)&addr, 
        sizeof(addr), 
        nullptr, nullptr, 
        nullptr, nullptr
        ))
    {
        trace_error_wsa("WSAConnect() failed");
        return INVALID_SOCKET;
    }

    return sock;
}

void message_loop(SOCKET sock)
{
    unsigned long bytes_send = 0;
    unsigned long bytes_recv = 0;

    WSABUF wsa_buf{};
    char buffer[MAX_MESSAGE_SIZE];
    unsigned long buffer_size = MAX_MESSAGE_SIZE;

    for (;;)
    {
        std::string message{};

        std::getline(std::cin, message);
        if (message.size() > MAX_MESSAGE_SIZE)
        {
            std::cout << "Max message size exceeded" << std::endl;
            continue;
        }

        wsa_buf.buf = message.data();
        wsa_buf.len = static_cast<unsigned long>(message.size());

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
            trace_error_wsa("WSASend() failed");
            break;
        }

        wsa_buf.buf = buffer;
        wsa_buf.len = buffer_size;

        // recv the response
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
            trace_error_wsa("WSASend() failed");
            break;
        }
    }
}

int main()
{
    WSADATA wsa_data;

    if (::WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        trace_error_wsa("WSAStartup() failed");
        return STATUS_FAILURE_I;
    }

    auto sock = client_connect(SOCKADDR_UN_PATH);
    if (INVALID_SOCKET == sock)
    {
        return STATUS_FAILURE_I;
    }

    trace_info("Connected successfully");

    message_loop(sock);

    ::closesocket(sock);
    ::WSACleanup();

    return STATUS_SUCCESS_I;
}