#pragma once
#include "HelperFunctions.hpp"
#include <charconv>

class RegisterFile
{
public:
	std::int32_t read(std::uint8_t registerInt8) const;
	void write(std::uint8_t registerInt8, std::int32_t registerValue);
	RegisterFile();

private:
	std::int32_t m_intRegister[32];
};

