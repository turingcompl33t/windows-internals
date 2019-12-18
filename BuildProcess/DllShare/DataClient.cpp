// DataClient.cpp
// Demo of simple data sharing via DLL.

#include <windows.h>

#include <iostream>

#pragma comment(lib, "DataShare.dll")

__declspec(dllimport) INT Counter;

INT main()
{
    INT value;

    for (;;)
    {
        std::cout << "Counter: " << Counter << '\n';
        std::cout << "Enter value to add (-1 to quit): ";
        std::cin >> value;
        if (value < 0)
        {
            break;
        }
        
        Counter += value;
    }

    return 0;
}