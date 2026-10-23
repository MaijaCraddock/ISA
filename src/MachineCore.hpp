#pragma once
#include <cstdint>
#include <iostream>
#include <charconv>
#include <deque>
#include <format> 
#include "Hart.hpp"
#include "RAM.hpp"

class MachineCore
{

public:
	MachineCore(std::uint32_t memBytes, std::uint32_t numHarts, 
		std::uint8_t mmuState, std::uint32_t bpRows, std::uint8_t bpDefault, 
		std::uint8_t extM, std::uint8_t extA);

	Hart& getHart(std::uint32_t hartID);

	void printMachineCore() const;
	
	void printRam(std::uint32_t start, std::uint32_t end, std::uint32_t bytesPerLine = 16);
	void loadRamAtIndex(const std::uint8_t* src, std::uint32_t elements, std::uint32_t addr);

private:
	std::uint32_t m_memBytes;
	std::uint32_t m_numHarts;
	std::uint8_t m_mmuState;

	RAM m_ram;
	std::deque<Hart> m_hartDeque;

};
