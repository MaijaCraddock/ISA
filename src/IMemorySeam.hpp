 #pragma once
#include <cstddef>
#include <cstdint>

enum class AccessType : std::uint8_t { Fetch, Load, Store };
enum class MemStatus : std::uint8_t { Ok, AccessFault, PageFault };

struct MemResult {
	std::uint32_t value = 0;   //little-endian
	std::uint32_t latency = 0;   //# of cycles
	MemStatus status = MemStatus::Ok;
};

//objects are interfaced from IMemorySeam
//classes will call a memory seam object with read, and that object will override its read function to perform correctly
class IMemorySeam {
public:
	virtual ~IMemorySeam() = default;
	virtual MemResult read(std::uint32_t addr, std::uint8_t size, AccessType type) = 0;
	virtual MemResult write(std::uint32_t addr, std::uint8_t size, std::uint32_t value) = 0;
	IMemorySeam* m_next ; //next object in the memory seam architecture, not implemented yet, as RAM is the bottom of the seam
};