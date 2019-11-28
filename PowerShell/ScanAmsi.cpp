// ScanAmsi.cpp
// Scan a specified input string with AMSI.
//
// EICAR string for testing:
//  "X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*"

#include <amsi.h>
#include <windows.h>

#include <cstdio>
#include <iostream>
#include <string_view>

#pragma comment(lib, "amsi.lib")
#pragma comment(lib, "ole32.lib")

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

std::string AmsiResultToString(const AMSI_RESULT result);

VOID LogInfo(const std::string_view msg);
VOID LogWarning(const std::string_view msg);
VOID LogError(const std::string_view msg, HRESULT result);

INT wmain(INT argc, WCHAR* argv[])
{
    LogInfo("ScanAmsi");

    HRESULT      ret;
    HAMSICONTEXT hAmsiContext;
    AMSI_RESULT  result;

    auto status = STATUS_SUCCESS_I;

    if (argc != 2)
    {
        LogWarning("Invalid arguments");
        LogWarning("Usage: ScanAmsi.exe <STRING>");
        status = STATUS_FAILURE_I;
        goto EXIT;
    }

    // initialize COM library
    ret = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(ret))
    {
        LogError("Failed to initialize COM library (CoInitializeEx())", ret);
        goto EXIT;
    }

    LogInfo("Successfully initialized the COM library");

    // initialize AMSI
    ret = AmsiInitialize(L"ScanAmsi", &hAmsiContext);
    if (FAILED(ret))
    {
        LogError("Failed to initialize AMSI (AmsiInitialize())", ret);
        status = STATUS_FAILURE_I;
        goto COM_CLEANUP_ONLY;
    }

    LogInfo("Successfully initialized AMSI context");

    // perform the scan
    ret = AmsiScanString(
        hAmsiContext,      // the context created by initialization
        argv[1],           // the string to scan
        L"MyAmsiScan",     // content name
        nullptr,           // no associated session
        &result            // result out-structure
        );

    if (FAILED(ret))
    {
        LogError("Failed to complete AMSI scan (AmsiScanString())", ret);
        goto CLEANUP_ALL;
    }

    LogInfo("Successfully completed AMSI scan of provided string");
    std::cout << "[+] Result: " << AmsiResultToString(result) << std::endl;

CLEANUP_ALL:
    AmsiUninitialize(hAmsiContext);
COM_CLEANUP_ONLY:
    CoUninitialize();
EXIT:
    return STATUS_SUCCESS_I;
}

std::string AmsiResultToString(const AMSI_RESULT result)
{
    // hooray for immediately invoked lambda expressions
    return [result]()
    {
        switch (result)
        {
        case AMSI_RESULT_CLEAN:
            return "AMSI_RESULT_CLEAN";
        case AMSI_RESULT_NOT_DETECTED:
            return "AMSI_RESULT_NOT_DETECTED";
        case AMSI_RESULT_BLOCKED_BY_ADMIN_START:
            return "ASMI_RESULT_BLOCKED_BY_ADMIN_START";
        case AMSI_RESULT_BLOCKED_BY_ADMIN_END:
            return "AMSI_RESULT_BLOCKED_BY_ADMIN_END";
        case AMSI_RESULT_DETECTED:
            return "AMSI_RESULT_NOT_DETECTED";
        default:
            return "AMSI_RESULT_NOT_RECOGNIZED";
        }
    }();
}

VOID LogInfo(const std::string_view msg)
{
    std::cout << "[+] " << msg << std::endl;
}

VOID LogWarning(const std::string_view msg)
{
    std::cout << "[-] " << msg << std::endl;
}

VOID LogError(const std::string_view msg, HRESULT result)
{
    LPSTR reason = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER 
        | FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        result,
        0,
        (LPSTR) &reason,
        0,
        NULL
    );
   
    std::cout << "[!] " << msg << '\n';
    std::cout << "[!]\tHRESULT CODE:    " << HRESULT_CODE(result) << '\n';
    std::cout << "[!]\tHRESULT MESSAGE: " << reason << '\n';
    std::cout << std::endl;

    // FormatMessageA allocates buffer for string
    LocalFree(reason);
}