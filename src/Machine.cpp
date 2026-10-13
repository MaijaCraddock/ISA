#include "ConfigParser.hpp"
#include "SignExtend.hpp"
#include "MachineCore.hpp"
#include "ELFParser.hpp"

int main(std::int32_t argc, char** argv) {
	ConfigParser conf = cfg::parseConfig(argc, argv, false);
	MachineCore machineCore(conf.m_memBytes, conf.m_numHarts, std::to_underlying(conf.m_mmuState));
	printf(" -- CONFIG PRINT (STRING FORM - FLAGS NOT IN ORDER) --\n\n");
	cfg::printConfigStringTypes(conf);
	printf("\n -- CONFIG PRINT (UNDERLYING VALUE FORM) --\n\n");
	cfg::printConfigUnderlyingTypes(conf);
	printf("\n -- MACHINE CORE PRINT --\n\n");
	machineCore.printMachineCore();
	printf("\n -- RAM PRINT (AFTER ELF LOADED) --\n\n");
	std::uint32_t e_entry = elf::parseELF(conf.m_inputElfPath, machineCore);
	
	//will change later, right now machineCore's numHarts is private
	//set all PC to e_entry (across all Harts)
	for (std::uint32_t currHart = 0; currHart < conf.m_numHarts; currHart++) {
		PC& currPC = machineCore.getHart(currHart).getPC();
		currPC.setPC(e_entry);
	}

	machineCore.printRam(0, conf.m_memBytes);
}

