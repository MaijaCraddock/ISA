#include "PC.hpp"

void PC::incPC()
{
	PC::ProgramCounter += k_ProgramCounterAlignment;
}

void PC::setPC(uint32_t newPC)
{
	PC::ProgramCounter = newPC;
}

void PC::offsetPC(int32_t offset)
{
	//could check overflow/underflow here, unsure of future API
	//add only if later API use doesnt imply guardrails
	PC::ProgramCounter += offset;
}

std::uint32_t PC::getPC() const
{
	return PC::ProgramCounter;
}
