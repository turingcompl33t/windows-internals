// AccessTokens.cpp
// Demo of some simple token management APIs.

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <sddl.h>
#include <tchar.h>

#include <iostream>

void LogInfo(const std::string& str);
void LogError(const std::string& str, DWORD dwGLE);

int wmain()
{
	std::cout << "Hello Access Tokens" << std::endl;

	BOOL bRet;

	// pseudo-handle
	HANDLE hThisProcess = GetCurrentProcess();

	HANDLE hProcessToken;
	bRet = OpenProcessToken(hThisProcess, TOKEN_READ, &hProcessToken);
	if (!bRet)
	{
		LogError("Failed to acquire handle to current process token", GetLastError());
		return -1;
	}

	TOKEN_TYPE TokenTypeData;
	DWORD dwOutSize;
	bRet = GetTokenInformation(
		hProcessToken, 
		TokenType, 
		&TokenTypeData, 
		sizeof(TokenTypeData), 
		&dwOutSize
	);
	if (!bRet)
	{
		LogError("Failed to query TokeUser information", GetLastError());
		return -1;
	}
	
	if (TokenTypeData == TokenPrimary)
	{
		LogInfo("This is a primary token");
	}
	else 
	{
		LogInfo("This is an impersonation token");
	}
}

void LogInfo(const std::string& str)
{
	std::cout << "[+] " << str << '\n';
}

void LogError(const std::string &str, DWORD dwGLE)
{
	std::cout << "[-] " << str << " (GLE: " << dwGLE << ")\n";
}