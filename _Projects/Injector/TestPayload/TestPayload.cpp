// TestPayload.cpp
// A simple test payload for DLL injection.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void DisplayMessageBox();

BOOL APIENTRY DllMain( 
    HMODULE hModule,
    DWORD   ul_reason_for_call,
    LPVOID  lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisplayMessageBox();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void DisplayMessageBox()
{
    int msgboxID = MessageBox(
        NULL,
        L"Cha-ching!",
        L"Injection Popup",
        MB_OK
    );
}

