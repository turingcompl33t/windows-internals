/*
 * TestTarget.cpp
 * Entry point for simple test target program.
 */

#include <iostream>
#include <Windows.h>

int main()
{
	while (TRUE)
	{
		std::cout << "Target Thread Spinning\n";
		Sleep(1000);
	}
    
	return 0;
}
