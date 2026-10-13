#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <iostream>

namespace helper {
	static constexpr int32_t int32_t_npos = -1;
	int32_t int32tStringFind(const std::string& source, const char target);
	int32_t int32tStringSize(const std::string& source);
	int32_t int32tStringSize(const std::string_view& source);
	[[noreturn]] void fatalError(const std::string& errMessage);
}
