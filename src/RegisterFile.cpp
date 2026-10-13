#include "RegisterFile.hpp"

std::int32_t RegisterFile::read(std::uint8_t registerInt8) const {

	if (registerInt8 > 31) {
		helper::fatalError("Register should be some number ranging 0 - 31 (" + std::to_string(registerInt8) + ")");
	}

	std::int32_t regValue = 0;
	if (registerInt8 != 0) {
		regValue = RegisterFile::m_intRegister[registerInt8];
	}
	return regValue;
}

void RegisterFile::write(std::uint8_t registerInt8, std::int32_t registerValue) {
	if (registerInt8 > 31) {
		helper::fatalError("Register should be some number ranging 0 - 31 (" + std::to_string(registerInt8) + ")");
	}

	if (registerInt8 != 0) {
		RegisterFile::m_intRegister[registerInt8] = registerValue;
	}
}

RegisterFile:: RegisterFile() {
	//x0 is init to zero, but API still handles read/write to x0, so this is mainly for raw data consistency
	RegisterFile::m_intRegister[0] = { 0 };
}

