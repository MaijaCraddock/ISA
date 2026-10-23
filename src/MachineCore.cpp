#include "MachineCore.hpp"

MachineCore::MachineCore(std::uint32_t memBytes, std::uint32_t numHarts, 
	std::uint8_t mmuState, std::uint32_t bpRows, std::uint8_t bpDefault, 
	std::uint8_t extM, std::uint8_t extA)
	: m_memBytes{ memBytes }
	, m_numHarts{ numHarts }
	, m_mmuState { mmuState }
	, m_ram		 { RAM(memBytes) }
{
	//couldnt dynamically allocate an array because Hart has no default constructor
	//must use a deque after disabling copying of Harts (Vector copies when resize happens, which shouldnt in this case, but is still unsafe)
	for (std::uint32_t i = 0; i < numHarts; ++i) {
		m_hartDeque.emplace_back(i, mmuState, m_ram, bpRows, bpDefault, extM, extA);
	}
}

Hart& MachineCore::getHart(std::uint32_t hartID)
{
	return m_hartDeque[hartID];
}

void MachineCore::printMachineCore() const {
	printf("Size of Ram:	 %" PRIu32 "\n"
		   "MMU State:       %" PRIu8 "\n"
		   "Number of Harts: %" PRIu32 "\n"
		, MachineCore::m_memBytes, MachineCore::m_mmuState, MachineCore::m_numHarts);
	printf(" -- HART DEQUE PRINT --\n");
	for (std::uint32_t hartIdx = 0; hartIdx < m_numHarts; hartIdx++) {
		printf("\n");
		m_hartDeque[hartIdx].print();
	}
}

void MachineCore::printRam(std::uint32_t start, std::uint32_t end, std::uint32_t bytesPerLine) {
	if (end > m_memBytes) { end = m_memBytes; }
	if (bytesPerLine == 0) { bytesPerLine = 16; }

	for (std::uint32_t addr = start; addr < end; addr++) {
		//label each row with the address of its first byte
		if ((addr - start) % bytesPerLine == 0) {
			std::cout << addr << " : ";
		}
		std::cout << std::format("{:02x} ", m_ram.read(addr, 1, AccessType::Load).value);
		if ((addr - start) % bytesPerLine == bytesPerLine - 1 || addr + 1 == end) {
			std::cout << std::endl;
		}
	}
}

void MachineCore::loadRamAtIndex(const std::uint8_t* src, std::uint32_t elements, std::uint32_t addr)
{
	m_ram.loadAtIndex(src, elements, addr);
}

