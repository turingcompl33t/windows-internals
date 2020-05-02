### Com Runtime Essentials

Activation, Registration, Class Factories, and Remoting.

**Client**

Build the client application:
    cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\WDL\include com_client.cpp /Fe:client.exe

**Server**

Build the server DLL:
    cl /EHsc /nologo /std:c++17 /W4 /LD /I %WIN_WORKSPACE%\_Deps\WDL\include registration.cpp com_server.cpp /Fe:server.dll /link /DEF:exports_server.def

Register the server DLL (must elevate):
    regsvr32 server.dll

Unregister the server DLL (must elevate):
    regsvr32 /u server.dll

**Proxy**

Compile IDL with MIDL compiler:
    midl hen.idl

Build proxy DLL:
    cl /W4 /DREGISTER_PROXY_DLL *.c /link /DLL rpcrt4.lib /out:proxy.dll /DEF:exports_proxy.def

Register the proxy DLL (must elevate):
    regsvr32 proxy.dll

Unregister the proxy DLL (must elevate):
    regsvr32 /u proxy.dll