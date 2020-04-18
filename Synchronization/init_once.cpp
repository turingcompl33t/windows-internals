// init_once.cpp
//
// Basic usage of Windows init-once initialization.
// 
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\Catch2 init_once.cpp

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <windows.h>