// string_test.cpp
//
// Testing wrappers for WRT strings.
//
// Build
//  cl /EHsc /nologo /std:c++17 /W4 /I %WIN_WORKSPACE%\_Deps\Catch2 /I %WIN_WORKSPACE%\_Deps\WDL\include string_test.cpp

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "string.hpp"

#pragma comment(lib, "windowsapp")

TEST_CASE("WRT string duplicate() increases the reference count of existing string")
{
    auto s1 = create_string(L"hello");

    REQUIRE(length(s1) == 5);
    REQUIRE_FALSE(empty(s1));

    auto s2 = duplicate(s1);

    REQUIRE(length(s2) == length(s1));
    REQUIRE(s2.get() == s1.get());
}

TEST_CASE("WRT string substring() allocates a new string with distinct underlying buffer")
{
    auto s1 = create_string(L"hello");

    REQUIRE(length(s1) == 5);
    REQUIRE_FALSE(empty(s1));

    auto s2 = substring(s1, 0);

    REQUIRE(length(s2) == length(s1));
    REQUIRE_FALSE(s2.get() == s1.get());
}