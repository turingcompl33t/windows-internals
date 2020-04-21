// com_server.hpp

#pragma once

#include <unknwn.h>

struct __declspec(uuid("6d5f1802-781c-4b95-8aeb-f33d07adb5db"))
Hen;

struct __declspec(uuid("42dc39d8-7594-486b-ae8b-5d49e6316304"))
IHen : IUnknown
{
	virtual void __stdcall Cluck() = 0;
};

// interface no longer utilized;
// replaced by generic class factory interface IClassFactory
struct __declspec(uuid("8cb49f49-96e8-4951-9e97-51585fb44f35"))
IHatchery : IUnknown
{
	virtual HRESULT __stdcall CreateHen(IHen** hen) = 0;
};