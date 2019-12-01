// WilThrow.cpp
// Demo of using WIL to wrap a call and enable exception-based error handling semantics.

#include <windows.h>
#include <shlwapi.h>
#include <tchar.h>

#include <iostream>
#include <string_view>

#include "wil\result.h"

#pragma comment(lib, "shlwapi.lib")

constexpr TCHAR VALID_FILENAME[]   = TEXT("C:\\Dev\\WinInternals\\WIL\\OpenMe.txt");
constexpr TCHAR INVALID_FILENAME[] = TEXT("C:\\Dev\\WinInternals\\WIL\\Fake.txt");

VOID LogInfo(const std::string_view msg);
VOID LogWarning(const std::string_view msg);

INT main()
{
    LogInfo("Hello WIL!");

    try
    {
        THROW_IF_WIN32_BOOL_FALSE(PathFileExists(VALID_FILENAME));
        LogInfo("VALID_FILENAME is a valid file path");
    }
    catch (wil::ResultException)
    {   
        LogWarning("VALID_FILENAME is an invalid file path");    
    }

    try
    {
        THROW_IF_WIN32_BOOL_FALSE(PathFileExists(INVALID_FILENAME));
        LogInfo("INVALID_FILENAME is a valid file path");
    }
    catch (wil::ResultException)
    {   
        LogWarning("INVALID_FILENAME is an invalid file path");
    }
}

VOID LogInfo(const std::string_view msg)
{
    std::cout << "[+] " << msg << std::endl;
}

VOID LogWarning(const std::string_view msg)
{
    std::cout << "[-] " << msg << std::endl;
}