#pragma once
#include <cstdint>
#include <iostream>
#include <charconv>
#include <vector>
#include <format> 
#include "Hart.hpp"

class MachineCore
{

public:
	MachineCore(std::uint32_t memBytes, std::uint32_t numHarts, std::uint8_t mmuState);
	~MachineCore();

	Hart& getHart(std::uint32_t hartID);

	void printMachineCore() const;

	template <typename T>
	void loadRamAtIndex(const T imm, std::uint32_t index);
	template <typename T>
	void loadRamAtIndex(const T* src, uint32_t elements, std::uint32_t index);
	
	void printRam(std::uint32_t start, std::uint32_t end, std::uint32_t bytesPerLine = 16);

private:
	//mainly used for debugging for now
	std::uint32_t m_memBytes;
	std::uint32_t m_numHarts;
	std::uint8_t m_mmuState;

	std::uint8_t* m_ram {};
	std::vector<Hart> m_hartVector;

};

//templated functions must be defined in .hpp file
template <typename T>
void MachineCore::loadRamAtIndex(const T imm,  std::uint32_t index) {

	//eviction not yet working, fatalError for now
	if (index + sizeof(T) / sizeof(uint8_t) > m_memBytes) {
		helper::fatalError("Not enough space in ram");
	}

	std::memcpy(&m_ram[index], &imm, sizeof(T));
}

//eviction not yet working
template <typename T>
void MachineCore::loadRamAtIndex(const T* src, uint32_t elements, std::uint32_t index) {
	//check if there is enough space in ram to hold elements at given index 
	if (index + sizeof(T) * elements / sizeof(uint8_t) > m_memBytes) {
		helper::fatalError("Not enough space in ram");
	}

	std::memcpy(&m_ram[index], src, elements * sizeof(T));
}