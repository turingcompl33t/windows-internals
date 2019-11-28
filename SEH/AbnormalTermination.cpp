// AbnormalTermination.cpp
// Demonstration of abnormal termination conditions.

#include <cstdio>
#include <windows.h>

INT main(INT argc, CHAR* argv[])
{
    BOOL status;
    const char t[] = "TRUE";
    const char f[] = "FALSE";

    if (argc != 2)
    {
        printf("Invalid arguments\n");
        printf("Usage: AbnormalTermination.exe <CODE>\n");
        return 1;
    }

    // weak i.e. zero validation
    int code = atoi(argv[1]);

    __try
    {
        if (0 == code)
        {
            printf("EXITING __try BLOCK WITH return\n");
            return 1;
        }
        else if (1 == code)
        {
            printf("EXITING __try BLOCK WITH __leave\n");
            __leave;
        }
        else 
        {
            // fall through
            printf("EXITING __try BLOCK WITH FALL-THROUGH\n");
        }
    }
    __finally
    {
        // termination handler

        // under what conditions was the __try block exited?
        status = AbnormalTermination();
        printf("TERMINATION HANDLER, ABNORMAL: %s\n", status ? t : f);
    }

    return 0;
}
