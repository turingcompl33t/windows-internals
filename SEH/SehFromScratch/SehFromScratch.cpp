// SehFromScratch.cpp
// Reimplementing basic SEH features via inline assembly.
//
// NOTE: obviously this is implementing frame-based SEH,
// thus this program only works on 32-bit platforms.

#include <windows.h>
#include <cstdio>

EXCEPTION_DISPOSITION
__cdecl
_except_handler(
	EXCEPTION_RECORD* ExceptionRecord,
	void* EstablisherFrame,
	CONTEXT* ContextRecord,
	void* DispatcherContext
)
{
	puts("Hello from exception handler");

	// increment EIP to skip invalid instruction
	ContextRecord->Eip += 3;

	// tell the OS the problem has been fixed
	return ExceptionContinueExecution;
}

int main()
{
	DWORD handler = (DWORD)_except_handler;

	puts("Registering SEH");

	__asm
	{                          // build new EXCEPTION_REGISTRATION structure
		push handler           // member: pointer to handler
		push FS : [0]          // member: pointer to previous handler 
		mov  FS : [0] , ESP    // install the new registration
	}

	puts("Null pointer access");

	__asm
	{                   // deliberately trigger a nullptr exception
		mov eax, 0
		mov[eax], 1
	}

	puts("After null pointer access");

	__asm
	{
		mov EAX, [ESP]            // grab the address of the registration structure
		mov FS : [0] , EAX        // reset to the previous registraton  
		add ESP, 8                // clear our EXCEPTION_REGISTRATION off the stack
	}

	return 0;
}