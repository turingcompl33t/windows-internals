// ErrorMode.cpp
// Demo of querying the process error mode.

#define UNICODE
#define _UNICODE

#include <windows.h>

#include <string>
#include <iostream>

std::string GetErrorModeString(const UINT errMode);

int main(int argc, char* argv[])
{
    UINT        ErrMode;
    std::string ModeString;

    ErrMode    = GetErrorMode();
    ModeString = GetErrorModeString(ErrMode);

    std::cout << "Process Error Mode: " << ModeString;
    std::cout << " (" << ErrMode << ")" << std::endl;

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
