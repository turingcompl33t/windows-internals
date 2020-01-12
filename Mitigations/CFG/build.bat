:: build.bat
:: Build application with/without CFG enabled.

cl /EHsc /nologo /FeCfgDisabled.exe HelloCfg.cpp
cl /EHsc /nologo /FeCfgEnabled.exe /guard:cf HelloCfg.cpp