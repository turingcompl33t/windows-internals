// WmiClient.cpp
// Dead-simple WMI client.

#define _WIN32_DCOM
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

int main(void)
{
    HRESULT res;

    res = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (S_OK != res)
    {
        std::cout << "CoInitializeEx() failed\n";
        return 1;
    }

    res = ::CoInitializeSecurity(
        NULL, 
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
        );

    if (S_OK != res)
    {
        std::cout << "CoInitializeSecurity() failed\n";
        ::CoUninitialize();
        return 1;
    }

    IWbemLocator *pLoc = NULL;
    res = ::CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID*) &pLoc
    );

    if (S_OK != res)
    {
        std::cout << "CoCreateInstance() failed\n";
        ::CoUninitialize();
        return 1;
    }

    IWbemServices *pSvc = NULL;
 
    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    res = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
        NULL,                    // User name. NULL = current user
        NULL,                    // User password. NULL = current
        0,                       // Locale. NULL indicates current
        NULL,                    // Security flags.
        0,                       // Authority (for example, Kerberos)
        0,                       // Context object 
        &pSvc                    // pointer to IWbemServices proxy
        );

    if (S_OK != res)
    {
        std::cout << "WBemLocator::ConnectServer() failed\n";
        pLoc->Release();
        ::CoUninitialize();
        return 1;
    }

    std::cout << "Connected to ROOT\\CIMV2 WMI namespace\n";

    // set security levels on the proxy
    res = CoSetProxyBlanket(
        pSvc,                        // Indicates the proxy to set
        RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
        NULL,                        // Server principal name 
        RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
        RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
        NULL,                        // client identity
        EOAC_NONE                    // proxy capabilities 
    );

    if (S_OK != res)
    {
        std::cout << "CoSetProxyBlanket() failed\n";
        pSvc->Release();
        pLoc->Release();
        ::CoUninitialize();
        return 1;
    }

    // finally, make a query
    // for example, query OS name
    IEnumWbemClassObject* pEnumerator = NULL;
    res = pSvc->ExecQuery(
        bstr_t("WQL"), 
        bstr_t("SELECT * FROM Win32_OperatingSystem"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
        NULL,
        &pEnumerator
        );

    if (S_OK != res)
    {
        std::cout << "IWbemServices::ExecQuery() failed\n";
        pSvc->Release();
        pLoc->Release();
        ::CoUninitialize();
        return 1;
    }

    // read the resulting query data

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
   
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(
            WBEM_INFINITE, 
            1, 
            &pclsObj, 
            &uReturn
            );

        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        // Get the value of the Name property
        hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        std::wcout << " OS Name : " << vtProp.bstrVal << std::endl;
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    // cleanup

    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    ::CoUninitialize();

    return 0;
}