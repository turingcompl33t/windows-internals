// Interactive.cpp
// Get and set process error mode.

#define UNICODE
#define _UNICODE

#include <windows.h>

#include <cstdio>
#include <string>
#include <iostream>

std::string GetErrorModeString(const UINT errMode);

int main(int argc, char *argv[])
{
    UINT mode;
    UINT curr;
    UINT prev;

    for (;;)
    {
        std::cout << "Enter error mode: ";
        std::cin >> mode;

        std::cout << "Setting process error mode to " << mode << '\n';

        prev = SetErrorMode(mode);
        curr = GetErrorMode();

        std::cout << "Previous error mode was " << prev << '\n';
        std::cout << "Current error mode reported ";
        std::cout << GetErrorModeString(curr) << " (" << curr << ")" << '\n';
        std::cout << '\n';
    }

    return 0;
}

std::string GetErrorModeString(const UINT errMode)
{
    return [errMode]()
    {
        switch (errMode)
        {
            case 0:
                return "SYSTEM_DEFAULT";
            case SEM_FAILCRITICALERRORS:
                return "SEM_FAILCRITICALERRORS";
            case SEM_NOALIGNMENTFAULTEXCEPT:
                return "SEM_NOALIGNMENTFAULTEXCEPT";
            case SEM_NOGPFAULTERRORBOX:
                return "SEM_NOGPFAULTERRORBOX";
            case SEM_NOOPENFILEERRORBOX:
                return "SEM_NOOPENFILEERRORBOX";
            default:
                return "UNRECOGNIZED";
        }
    }();
}