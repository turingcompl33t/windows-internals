// symmetric.cpp
//
// Demo of basic symmetric cryptography with CNG via WDL.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include symmetric.cpp

#include <wdl/crypto/symmetric.hpp>

#include <string>
#include <iostream>

#pragma comment(lib, "bcrypt")

namespace crypto = wdl::crypto;

int main()
{   
    auto const secret  = std::string{"some shared secret"};
    auto const message = std::string{"secure message"};

	auto const shared = crypto::create_shared_secret(crypto::buffer(secret));

	auto const ciphertext = crypto::encrypt_message(crypto::buffer(shared), crypto::buffer(message));
	auto const plaintext  = crypto::decrypt_message(crypto::buffer(shared), crypto::buffer(ciphertext));

	auto msg = std::string(
		reinterpret_cast<char*>(const_cast<crypto::byte_t*>(&plaintext[0])),
		plaintext.size());

	std::cout << msg << std::endl;
}