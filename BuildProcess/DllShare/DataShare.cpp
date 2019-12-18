// DataShare.cpp
// Demo of simple data sharing via DLL.

#include <windows.h>

BOOL APIENTRY DllMain(
    _In_ HINSTANCE hInstDll,
    _In_ DWORD fdwReason,
    _In_ LPVOID lpReserved
)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}

#pragma data_seg("Shared", "RWS")
__declspec(dllexport) INT Counter = 0;
#pragma data_seg()

#pragma comment(linker, "/section:Shared,RWS")