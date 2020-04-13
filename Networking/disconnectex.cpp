// disconnectex.cpp
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 disconnectex.cpp

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <MSWSock.h>

#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

int main()
{
    auto data = WSADATA{};
    ::WSAStartup(MAKEWORD(2, 2), &data);

    auto sock1 = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    auto sock2 = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

    if (INVALID_SOCKET == sock1 ||
        INVALID_SOCKET == sock2)
    {
        printf("Failed to open sockets\n");
        ::WSACleanup();
        return STATUS_FAILURE_I;
    }

    GUID guid = WSAID_DISCONNECTEX;

    auto ptr1       = UINT_PTR{};
    auto ptr2       = UINT_PTR{};
    auto bytes_ret = unsigned long{};

    if (SOCKET_ERROR == ::WSAIoctl(
        sock1, 
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guid,
        sizeof(guid),
        &ptr1,
        sizeof(ptr1),
        &bytes_ret,
        nullptr,
        nullptr))
    {
        printf("WSAIoctl() failed: %u\n", ::WSAGetLastError());
        ::WSACleanup();
        return STATUS_FAILURE_I;
    }

    printf("sock1 pointer = %p (%u bytes)\n", (void*)ptr1, bytes_ret);

    if (SOCKET_ERROR == ::WSAIoctl(
        sock2, 
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guid,
        sizeof(guid),
        &ptr2,
        sizeof(ptr2),
        &bytes_ret,
        nullptr,
        nullptr))
    {
        printf("WSAIoctl() failed: %u\n", ::WSAGetLastError());
        ::WSACleanup();
        return STATUS_FAILURE_I;
    }

    printf("sock2 pointer = %p (%u bytes)\n", (void*)ptr2, bytes_ret);

    ::WSACleanup();
    return STATUS_SUCCESS_I;
}