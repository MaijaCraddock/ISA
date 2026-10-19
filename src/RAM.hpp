#pragma once
#include <cstdint>
#include <memory>
#include "IMemorySeam.hpp"

class RAM : public IMemorySeam {
public:
	static constexpr std::uint32_t kLatencyCycles = 1;
	RAM(std::uint32_t sizeBytes);
	std::uint32_t getSize() const;
	MemResult read(std::uint32_t addr, std::uint8_t size, AccessType type) override;
	MemResult write(std::uint32_t addr, std::uint8_t size, std::uint32_t value) override;
	std::uint8_t loadAtIndex(const std::uint8_t* src, std::uint32_t elements, std::uint32_t addr); // ELF return bool (uint8_t)
private:
	std::uint8_t inBounds(std::uint32_t addr, std::uint32_t count) const; //return bool (uint8_t)
	std::uint32_t m_size;
	std::unique_ptr<std::uint8_t[]> m_bytes;
};