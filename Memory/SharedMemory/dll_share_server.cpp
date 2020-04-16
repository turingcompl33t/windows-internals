// dll_share_server.cpp
//
// Demo of simple data sharing via DLL.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /D_USRDLL /D_WINDLL dll_share_server.cpp /link /DLL /OUT:dll_share_server.dll

#include <windows.h>

BOOL APIENTRY DllMain(
    _In_ HINSTANCE,
    _In_ unsigned long reason,
    _In_ void*
)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}

#pragma data_seg("shared", "RWS")
__declspec(dllexport) unsigned long g_counter = 0;
#pragma data_seg()

#pragma comment(linker, "/section:shared,RWS")