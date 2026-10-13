#pragma once
#include <cstdint>
#include <string>
#include <fstream>
#include <bit>
#include <cstring>
#include "MachineCore.hpp"

namespace elf {
	std::uint32_t parseELF(std::string filename, MachineCore& machine);
}