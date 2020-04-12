// hash.cpp
//
// Demo of basic hash computatiom with CNG via WDL.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include hash.cpp

#include <wdl/crypto/hash.hpp>

#include <wrl.h>
#include <Shlwapi.h>
#include <iostream>

#pragma comment(lib, "bcrypt")
#pragma comment(lib, "shlwapi")

using namespace Microsoft::WRL;
namespace crypto = wdl::crypto;

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto BUFFER_SIZE = 4096;

void dump_bytes(unsigned char* buffer, size_t size)
{
    constexpr const auto ROW_LENGTH = 16;

    std::cout << std::hex;

    for (auto i = 0; i < size; i += ROW_LENGTH)
    {
        for (auto j = i; j < (i + ROW_LENGTH) && j < size; ++j)
        {
            std::cout << "0x" << static_cast<unsigned>(buffer[j]) << " ";
        }
        std::cout << '\n';
    }

    std::cout << std::dec << std::flush;
}

int main()
{
	auto provider = crypto::open_provider(BCRYPT_SHA512_ALGORITHM);

	auto hash = crypto::create_hash(provider);

	auto file = ComPtr<IStream>{};

	VERIFY_(S_OK, ::SHCreateStreamOnFileEx(
		L"C:\\Windows\\explorer.exe",
		0, 0,
		false,
		nullptr,
		file.GetAddressOf()));

	unsigned char buffer[BUFFER_SIZE];
	auto size = unsigned long{};

	while (SUCCEEDED(file->Read(buffer, sizeof(buffer), &size)) && size)
	{
		crypto::combine_hash(hash, buffer, size);
	}

	crypto::get_property(hash.get(), BCRYPT_HASH_LENGTH, size);

	auto value = crypto::bytes_t(size);
	crypto::get_hash_value(hash, &value[0], size);

    dump_bytes(&value[0], size);

	SecureZeroMemory(&value[0], size);

    return STATUS_SUCCESS_I;
}