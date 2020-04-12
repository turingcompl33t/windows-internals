// random.cpp
//
// Demo of basic random number generation with CNG via WDL.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include random.cpp

#include <wdl/crypto/random.hpp>
#include <iostream>

#pragma comment(lib, "bcrypt")

namespace crypto = wdl::crypto;

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

constexpr const auto BUFFER_SIZE = 32;

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
	auto provider = crypto::open_provider(BCRYPT_RNG_ALGORITHM);

	unsigned char buffer[BUFFER_SIZE];
	crypto::random(provider, buffer);

    dump_bytes(buffer, BUFFER_SIZE);

    return STATUS_SUCCESS_I;
}