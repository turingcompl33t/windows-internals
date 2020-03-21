// cryptography.cpp
// Powering up Visual C++ with the Windows CNG cryptography API.

#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt")
#include <wrl.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi")

#include <vector>
#include <string>
#include <iostream>

#include <wdl/debug.h>
#include <wdl/unique_handle.h>

using namespace Microsoft::WRL;

/* ---------------------------------------------------------------------------- 
 *	Utilities
 */

struct status_exception
{
	NTSTATUS code;
	
	status_exception(NTSTATUS result)
		: code{ result }
	{}
};

void check(NTSTATUS const status)
{
	if (ERROR_SUCCESS != status)
	{
		throw status_exception{ status };
	}
}

wdl::provider_handle open_provider(wchar_t const* algorithm)
{
	auto p = wdl::provider_handle{};

	check(::BCryptOpenAlgorithmProvider(
		p.get_address_of(),
		algorithm,
		nullptr,
		0
		));

	return p;
}

template <typename T>
void get_property(
	BCRYPT_HANDLE handle,
	wchar_t const* name,
	T& value
)
{
	auto bytes_copied = ULONG{};

	check(::BCryptGetProperty(
		handle,
		name,
		reinterpret_cast<PUCHAR>(&value),
		sizeof(T),
		&bytes_copied,
		0
	));
}

/* ----------------------------------------------------------------------------
 *	Random Number Generation
 */

void random(
	wdl::provider_handle const& p,
	void* buffer,
	unsigned size
	)
{
	check(::BCryptGenRandom(
		p.get(),
		static_cast<PUCHAR>(buffer),
		size,
		0
		));
}

template <typename T, unsigned Count>
void random(
	wdl::provider_handle const& p,
	T(&buffer)[Count]
)
{
	random(p, buffer, sizeof(T) * Count);
}

template <typename T>
void random(wdl::provider_handle const& p, T& buffer)
{
	random(p, &buffer, sizeof(T));
}

void random_demo()
{
	auto p = open_provider(BCRYPT_RNG_ALGORITHM);

	unsigned buffer[10];
	random(p, buffer);
}

/* ----------------------------------------------------------------------------
 *	Hash Computation
 */

wdl::hash_handle create_hash(wdl::provider_handle const& p)
{
	auto h = wdl::hash_handle{};

	check(::BCryptCreateHash(
		p.get(),
		h.get_address_of(),
		nullptr,
		0,
		nullptr,
		0,
		0
		));

	return h;
}

void combine(
	wdl::hash_handle const& h,
	void const* buffer,
	unsigned size
	)
{
	check(::BCryptHashData(
		h.get(),
		static_cast<PUCHAR>(const_cast<void*>(buffer)),
		size,
		0
		));
}

void get_hash_value(
	wdl::hash_handle const& h,
	void* buffer,
	unsigned size
	)
{
	check(::BCryptFinishHash(
		h.get(),
		static_cast<PUCHAR>(buffer),
		size,
		0
		));
}

void hash_demo()
{
	auto p = open_provider(BCRYPT_SHA512_ALGORITHM);

	auto h = create_hash(p);

	auto file = ComPtr<IStream>{};

	VERIFY_(S_OK, ::SHCreateStreamOnFileEx(
		L"C:\\Windows\\explorer.exe",
		0,
		0,
		false,
		nullptr,
		file.GetAddressOf()
		));

	BYTE buffer[4096];
	auto size = ULONG{};

	while (SUCCEEDED(file->Read(buffer, sizeof(buffer), &size)) && size)
	{
		combine(h, buffer, size);
	}

	get_property(h.get(), BCRYPT_HASH_LENGTH, size);

	auto value = std::vector<BYTE>(size);
	get_hash_value(h, &value[0], size);

	SecureZeroMemory(&value[0], size);
}

/* ----------------------------------------------------------------------------
 *	Symmetric Encryption
 */

wdl::key_handle create_key(
	wdl::provider_handle const& p,
	void const* secret,
	unsigned size
	)
{
	auto k = wdl::key_handle{};

	check(::BCryptGenerateSymmetricKey(
		p.get(),
		k.get_address_of(),
		nullptr,
		0,
		static_cast<PUCHAR>(const_cast<void*>(secret)),
		size,
		0
		));

	return k;
}

ULONG encrypt(
	wdl::key_handle const& k,
	void const* plaintext,
	unsigned plaintext_size,
	void* ciphertext,
	unsigned ciphertext_size,
	unsigned flags
)
{
	auto bytes_copied = ULONG{};

	check(::BCryptEncrypt(
		k.get(),
		static_cast<PUCHAR>(const_cast<void*>(plaintext)),
		plaintext_size,
		nullptr,
		nullptr,
		0,
		static_cast<PUCHAR>(ciphertext),
		ciphertext_size,
		&bytes_copied,
		flags
		));

	return bytes_copied;
}

ULONG decrypt(
	wdl::key_handle const& k,
	void const* ciphertext,
	unsigned ciphertext_size,
	void* plaintext,
	unsigned plaintext_size,
	unsigned flags
)
{
	auto bytes_copied = ULONG{};

	check(::BCryptDecrypt(
		k.get(),
		static_cast<PUCHAR>(const_cast<void*>(ciphertext)),
		ciphertext_size,
		nullptr,
		nullptr,
		0,
		static_cast<PUCHAR>(plaintext),
		plaintext_size,
		&bytes_copied,
		flags
		));

	return bytes_copied;
}

std::vector<BYTE> create_shared_secret(
	std::string const& secret
	)
{
	auto p = open_provider(BCRYPT_SHA256_ALGORITHM);

	auto h = create_hash(p);

	combine(h, secret.c_str(), secret.size());

	auto size = unsigned{};

	get_property(h.get(), BCRYPT_HASH_LENGTH, size);

	auto value = std::vector<BYTE>(size);

	get_hash_value(h, &value[0], size);

	return value;
}

std::vector<BYTE> encrypt_message(
	std::vector<BYTE> const& shared,
	std::string const& plaintext
	)
{
	auto p = open_provider(BCRYPT_3DES_ALGORITHM);

	auto k = create_key(
		p,
		&shared[0],
		shared.size()
		);

	auto size = encrypt(
		k,
		plaintext.c_str(),
		plaintext.size(),
		nullptr,
		0,
		BCRYPT_BLOCK_PADDING
		);

	std::vector<BYTE> ciphertext(size);

	encrypt(
		k,
		plaintext.c_str(),
		plaintext.size(),
		&ciphertext[0],
		size,
		BCRYPT_BLOCK_PADDING
		);

	return ciphertext;
}

std::string decrypt_message(
	std::vector<BYTE> const& shared,
	std::vector<BYTE> const& ciphertext
	)
{
	auto p = open_provider(BCRYPT_3DES_ALGORITHM);

	auto k = create_key(
		p,
		&shared[0],
		shared.size()
		);

	auto size = decrypt(
		k,
		&ciphertext[0],
		ciphertext.size(),
		nullptr,
		0,
		BCRYPT_BLOCK_PADDING
		);

	auto plaintext = std::string(size, '\0');

	decrypt(
		k,
		&ciphertext[0],
		ciphertext.size(),
		&plaintext[0],
		size,
		BCRYPT_BLOCK_PADDING
		);

	plaintext.resize(strlen(plaintext.c_str()));

	return plaintext;
}

void symmetric_demo()
{
	auto shared = create_shared_secret("some shared secret");

	auto ciphertext = encrypt_message(shared, "secure message");

	auto plaintext = decrypt_message(shared, ciphertext);

	std::cout << plaintext;
}

/* ----------------------------------------------------------------------------
 *	Entry Point
 */

int wmain()
{
	random_demo();
	hash_demo();
	symmetric_demo();
}