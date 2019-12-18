cl /D_USRDLL /D_WINDLL DataShare.cpp /link /DLL /OUT:DataShare.dll
cl /EHsc /nologo DataClient.cpp