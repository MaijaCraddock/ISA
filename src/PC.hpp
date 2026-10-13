#pragma once
#include <cstdint>
#include "HelperFunctions.hpp"

class PC
{
public:
	// Im honestly not sure which of these will be used/unused yet, 
	// but will remove/change their logic in the future when they become redundant
	void incPC();
	void setPC(uint32_t newPC);
	void offsetPC(int32_t offset);
	std::uint32_t getPC() const;
private:
	inline static constexpr std::uint8_t k_ProgramCounterAlignment = 4;
	std::uint32_t ProgramCounter;
};

