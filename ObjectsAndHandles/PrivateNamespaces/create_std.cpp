// create_std.cpp
//
// Create a private namespace accessible to all standard users.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include create_std.cpp

#include <windows.h>

#include <iostream>
#include <string_view>

#include <wdl/handle/generic.hpp>
#include <wdl/handle/private_namespace.hpp>

#pragma comment(lib, "advapi32")

using null_handle                = wdl::handle::null_handle;
using private_namespace_handle   = wdl::handle::private_namespace_handle;
using boundary_descriptor_handle = wdl::handle::boundary_descriptor_handle;

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

void error(
    std::string_view msg,
    unsigned long const code = ::GetLastError()
    )
{
    std::cout << "[-] " << msg
        << "Code: " << std::hex << code << std::dec
        << std::endl;
}

int main()
{
    // create a boundary descriptor
    auto bd = boundary_descriptor_handle{
        ::CreateBoundaryDescriptorW(L"MyDescriptor", 0) };
    if (!bd)
    {
        error("CreateBoundaryDescriptor() failed");
        return STATUS_FAILURE_I;
    }

    // add standard user SID to boundary descriptor
    unsigned char buffer[SECURITY_MAX_SID_SIZE];

    auto psid = reinterpret_cast<PSID>(buffer);
    auto sid_len = unsigned long{};
    ::CreateWellKnownSid(
        WinBuiltinUsersSid, 
        nullptr, 
        psid, 
        &sid_len);

    ::AddSIDToBoundaryDescriptor(bd.get_address_of(), psid);

    // create the private namespace
    auto ns = private_namespace_handle{
        ::CreatePrivateNamespaceW(nullptr, bd.get(), L"Private") };
    if (!ns)
    {
        error("CreatePrivateNamespace() failed");
        return STATUS_FAILURE_I;
    }

    // create some named object in the private namespace
    auto mutex = null_handle{
        ::CreateMutexW(nullptr, FALSE, L"Private\\my-private-mutex") };
    if (!mutex)
    {
        error("CreateMutex() failed");
        return STATUS_FAILURE_I;
    }

    std::cout << "[+] Succesfully created named mutex in private namespace\n";
    std::cin.get();

    return STATUS_SUCCESS_I;
}