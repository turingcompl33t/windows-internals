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
    auto secret  = crypto::to_bytes("some shared secret");
    auto message = crypto::to_bytes("secure message");

	auto shared = crypto::create_shared_secret(secret);

	auto ciphertext = crypto::encrypt_message(shared, message);
	auto plaintext  = crypto::decrypt_message(shared, ciphertext);

	auto msg = crypto::from_bytes(plaintext);

	std::cout << msg << std::endl;
}