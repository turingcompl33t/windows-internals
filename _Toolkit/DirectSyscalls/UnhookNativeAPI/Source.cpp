/*
 * Source.cpp
 * Entry point for native API unhooking demo.
 */

#include <tchar.h>
#include <iostream>
#include <windows.h>

#include "LibDirectSyscalls.h"

int main()
{
    std::cout << "Hello World!\n";
}

BOOL UnhookNativeAPI()
{
	return TRUE;
}
