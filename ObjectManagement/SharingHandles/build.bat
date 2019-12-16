:: build.bat
:: Build both parent and child applications.

cl /EHsc /nologo InheritParent.cpp
cl /EHsc /nologo InheritChild.cpp