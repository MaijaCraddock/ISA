#include "MachineCore.hpp"

MachineCore::MachineCore(std::uint32_t memBytes, std::uint32_t numHarts, std::uint8_t mmuState)
{
	MachineCore::m_mmuState = mmuState;
	MachineCore::m_memBytes = memBytes;
	MachineCore::m_ram = new std::uint8_t[memBytes]{};
	//couldnt dynamically allocate an array because Hart has no default constructor
	MachineCore::m_numHarts = numHarts;
	MachineCore::m_hartVector.reserve(numHarts);
	for (std::uint32_t i = 0; i < numHarts; ++i) {
		m_hartVector.push_back(Hart(i, mmuState));
	}
}

//ignoring rule of three...
MachineCore::~MachineCore()
{
	delete[] MachineCore::m_ram;
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
	std::uint32_t count = 0;
	std::cout << start << " : ";
	for (std::uint32_t idx = start; idx <= end; idx++) {
		std::cout << std::format("{:02x} ", m_ram[idx]);
		count++;
		if (count >= bytesPerLine) { 
			std::cout << std::endl << idx << " : ";
			count = 0; 
		}
	}
}

//loadRamAtIndex functions located in MachineCore.hpp (templated)