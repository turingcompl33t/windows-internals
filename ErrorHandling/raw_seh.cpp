/*
 * Main.cpp
 * Exploring SEH.
 */

#include <cstdio>
#include <tchar.h>
#include <Windows.h>

int const ConstantZero = 0;

int Case0(void);
int Case1(void);

_Function_class_(EXCEPTION_ROUTINE)
EXCEPTION_DISPOSITION MyExceptionHandler(
	EXCEPTION_RECORD* ExceptionRecord,
	void* EstablisherFrame,
	CONTEXT* ContextRecord,
	void* DispatcherContext
);

int _tmain(int argc, _TCHAR* argv[])
{
	// swap in test cases here
	return Case1();
}

// Case 0
// Raise a GP exception and do nothing to address it.
int Case0(void)
{
	printf("ConstantZero = %d\n", ConstantZero);

	const_cast<int&>(ConstantZero) = 1;

	printf("ConstantZero = %d\n", ConstantZero);

	return 0;
}

int Case1(void)
{
	NT_TIB* TIB = (NT_TIB*)NtCurrentTeb();

	// splice the new handler into exception handler list
	EXCEPTION_REGISTRATION_RECORD Registration;
	Registration.Handler = &MyExceptionHandler;
	Registration.Next = TIB->ExceptionList;
	TIB->ExceptionList = &Registration;

	printf("ConstantZero = %d\n", ConstantZero);

	const_cast<int&>(ConstantZero) = 1;

	printf("ConstantZero = %d\n", ConstantZero);

	// and splice the handler out of list when complete
	TIB->ExceptionList = TIB->ExceptionList->Next;

	return 0;
}

_Function_class_(EXCEPTION_ROUTINE)
EXCEPTION_DISPOSITION MyExceptionHandler(
	EXCEPTION_RECORD* ExceptionRecord,
	void* EstablisherFrame,
	CONTEXT* ContextRecord,
	void* DispatcherContext
)
{
	printf(
		"An exception occured at address 0x%p, with ExceptionCode = 0x%08x!\n",
		ExceptionRecord->ExceptionAddress,
		ExceptionRecord->ExceptionCode);

	return ExceptionContinueSearch;
}
