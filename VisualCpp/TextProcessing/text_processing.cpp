// TextProcessing.cpp
// Powering up Visual C++ with performant text processing.

#include <windows.h>
#include <ppl.h>

#include <map>
#include <regex>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>

#include <wdl/io/io.h>

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

using word_map   = std::map<std::string, unsigned>;
using time_point = std::chrono::high_resolution_clock::time_point;

time_point time_now();
float time_elapsed(const time_point& start);

int wmain(int argc, wchar_t* argv[])
{
	const auto rx = std::regex{ "\\w+" };

	// utilize the combinable template provided
	// by the concurrency runtime
	auto shared = concurrency::combinable<word_map>{};

	const auto start = time_now();

	concurrency::parallel_for_each(argv + 1, argv + argc, [&](const wchar_t* name)
	{
		// map the file into memory
		auto file = wdl::mapped_file{ name };

		if (!file) return;

		auto& result = shared.local();

		// iterate over tokens parsed from the mapped file
		// with the aid of the standard library's regex iterator
		for (auto it = std::cregex_token_iterator{ wdl::begin(file), wdl::end(file), rx };
			it != std::cregex_token_iterator{};
			++it)
		{
			++result[*it];
		}
	});

	// the final word map
	auto result = word_map{};

	// combine the results into the final map
	shared.combine_each([&](const word_map& local)
	{
		for (const auto& w : local)
		{
			result[w.first] += w.second;
		}
	});

	auto elapsed = time_elapsed(start);

	std::cout << "Words: " << result.size() << '\n';
	std::cout << "Time:  " << elapsed << '\n';

	return STATUS_SUCCESS_I;
}

time_point time_now()
{
	return std::chrono::high_resolution_clock::now();
}

float time_elapsed(const time_point& start)
{
	return std::chrono::duration_cast<std::chrono::duration<float>>
		(time_now() - start).count();
}