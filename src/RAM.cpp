#include "RAM.hpp"
#include "HelperFunctions.hpp"
#include <cstring>

RAM::RAM(std::uint32_t n) 
	: m_size(n)
	, m_bytes(new std::uint8_t[n]) {}

std::uint32_t RAM::getSize() const
{
	return m_size;
}

std::uint8_t RAM::inBounds(std::uint32_t addr, std::uint32_t count) const {
	return addr <= m_size && count <= m_size - addr;   // addr + count, accounts for overflow
}

MemResult RAM::read(std::uint32_t addr, std::uint8_t size, AccessType) {
	MemResult r; 
	r.latency = kLatencyCycles;
	
	if (!inBounds(addr, size)) 
	{
		r.status = MemStatus::AccessFault;
		return r; 
	}


	// shift bytes in little endian
	for (std::uint8_t i = 0; i < size; ++i)
	{
		r.value |= static_cast<std::uint32_t>(m_bytes[addr + i]) << (8u * i);
	}

	return r;
}

MemResult RAM::write(std::uint32_t addr, std::uint8_t size, std::uint32_t value)
{
	MemResult r;
	r.latency = kLatencyCycles;

	if (!inBounds(addr, size)) {
		r.status = MemStatus::AccessFault;
		return r;
	}

	// shift bytes out little endian
	for (std::uint8_t i = 0; i < size; ++i) {
		m_bytes[addr + i] = static_cast<std::uint8_t>(value >> (8u * i));
	}
	return r;
}

void RAM::loadAtIndex(const std::uint8_t* src, uint32_t elements, std::uint32_t addr) 
{
	if (!inBounds(addr, elements)) {
		helper::fatalError("Not enough space in RAM");
	}

	//already in little endian (read from ELF)
	std::memcpy(&m_bytes[addr], src, elements);
}