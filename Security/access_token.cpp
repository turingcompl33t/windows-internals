// access_token.cpp
//
// Demo of some simple token management APIs.
//
// Build
//	cl /EHsc /nologo /std:c++17 /W4 access_token.cpp

#include <windows.h>
#include <sddl.h>

#pragma comment(lib, "advapi32")

#include <iostream>

constexpr auto const STATUS_SUCCESS_I = 0x0;
constexpr auto const STATUS_FAILURE_I = 0x1;

void info(std::string const& msg)
{
	std::cout << "[+] " << msg << '\n';
}

void error(
	std::string const& msg, 
	unsigned long const code = ::GetLastError()
	)
{
	std::cout << "[-] " << msg
		<< " Code: << " << code
		<< std::endl;
}

int main()
{
	auto this_token = HANDLE{};
	if (!::OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &this_token))
	{
		error("Failed to acquire handle to current process token");
		return STATUS_FAILURE_I;
	}

	auto token_type = TOKEN_TYPE{};
	auto out_bytes = unsigned long{};
	if (!::GetTokenInformation(
		this_token, 
		TokenType, 
		&token_type, 
		sizeof(token_type), 
		&out_bytes))
	{
		error("Failed to query TokeUser information");
		return STATUS_FAILURE_I;
	}
	
	if (TokenPrimary == token_type)
	{
		info("This is a primary token");
	}
	else 
	{
		info("This is an impersonation token");
	}

	::CloseHandle(this_token);

	return STATUS_SUCCESS_I;
}