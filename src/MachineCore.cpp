#include "MachineCore.hpp"

MachineCore::MachineCore(std::uint32_t memBytes, std::uint32_t numHarts, std::uint8_t mmuState, std::uint32_t bpRows, std::uint8_t bpDefault)
	: m_mmuState { mmuState }
	, m_memBytes { memBytes }
	, m_ram		 { RAM(memBytes) }
	, m_numHarts { numHarts }
{
	//couldnt dynamically allocate an array because Hart has no default constructor
	m_hartVector.reserve(numHarts);
	for (std::uint32_t i = 0; i < numHarts; ++i) {
		m_hartVector.push_back(Hart(i, mmuState, m_ram, bpRows, bpDefault));
	}
}


Hart& MachineCore::getHart(std::uint32_t hartID)
{
	return m_hartVector[hartID];
}

void MachineCore::printMachineCore() const {
	printf("Size of Ram:	 %" PRIu32 "\n"
		   "MMU State:       %" PRIu8 "\n"
		   "Number of Harts: %" PRIu32 "\n"
		, MachineCore::m_memBytes, MachineCore::m_mmuState, MachineCore::m_numHarts);
	printf(" -- HART VECTOR PRINT --\n");
	for (std::uint32_t hartIdx = 0; hartIdx < m_numHarts; hartIdx++) {
		printf("\n");
		Hart currHart = m_hartVector[hartIdx];
		currHart.print();

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

