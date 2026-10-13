#include "ELFParser.hpp"
#include "HelperFunctions.hpp"

namespace {
	//expected bytes in header
	static constexpr std::uint8_t e_identTargetArray[4]{ 0x7f, 0x45, 0x4c, 0x46 };
	static constexpr std::uint8_t e_typeTargetArray[2]{0x02, 0x00};
	static constexpr std::uint8_t e_machineTargetArray[2]{0xf3, 0x00};
	static constexpr std::uint32_t e_identTarget = std::bit_cast<std::uint32_t>(e_identTargetArray);		//0x7f E L F - in whatever endian-ness runs this
	static constexpr std::uint8_t e_bitsizeTarget = 0x1;													//32bit
	static constexpr std::uint16_t e_typeTarget = std::bit_cast<std::uint16_t>(e_typeTargetArray);			//executable
	static constexpr std::uint16_t e_machineTarget = std::bit_cast<std::uint16_t>(e_machineTargetArray);	//243 / RISC-V

	static constexpr std::uint8_t p_typeTargetArray[4]{ 0x01, 0x00, 0x00, 0x00 };
	static constexpr std::uint32_t p_typeTarget = std::bit_cast<std::uint32_t>(p_typeTargetArray);			//1   / PT_LOAD


	//Checks the validity of the first 52 bytes of the ELF and makes sure the header is valid
	void isValidEH(std::ifstream& fin, std::string filename) {
		if (!fin.is_open()) {
			helper::fatalError("Could not open ELF (" + filename + ")");
		}
		fin.seekg(0, std::ios::end);
		std::uint32_t finSize = static_cast<std::uint32_t>(fin.tellg());
		if (finSize < 52) {
			helper::fatalError("ELF file is less than 52 bytes (" + filename + ")");
		}
		
		//read up to 4 bytes at a time depending on the size of the field
		//bit cast it to field later
		std::uint8_t readBuffer[4];

		//we can bitcast readBuffer on 1 and 4 byte fields, but we need a different buffer for 2 bytes
		std::uint8_t readBufferu16[2];

		//e_ident offset = 0, size = 4
		fin.seekg(0, std::ios::beg);
		fin.read(reinterpret_cast<char*>(readBuffer), sizeof(e_identTarget));
		
		if (std::bit_cast<uint32_t>(readBuffer) != e_identTarget) {
			helper::fatalError("ELF header e_ident not valid (" + std::to_string(std::bit_cast<uint32_t>(readBuffer))+ ")" );
		}

		//e_bitsize offset = 4, size = 1 (no seek)
		fin.read(reinterpret_cast<char*>(readBuffer), sizeof(e_bitsizeTarget));

		if (std::bit_cast<uint8_t>(readBuffer[0]) != e_bitsizeTarget) {
			helper::fatalError("ELF header e_bitsize not valid (" + std::to_string(readBuffer[0]) + ")");
		}

		//e_type offset = 16, size = 2
		fin.seekg(16, std::ios::beg);
		fin.read(reinterpret_cast<char*>(readBufferu16), sizeof(e_typeTarget));

		if (std::bit_cast<uint16_t>(readBufferu16) != e_typeTarget) {
			helper::fatalError("ELF header e_type not valid (" + std::to_string(std::bit_cast<uint16_t>(readBufferu16)) + ")");
		}

		//e_machine offset = 18, size = 2 (no seek)
		fin.seekg(18, std::ios::beg);
		fin.read(reinterpret_cast<char*>(readBufferu16), sizeof(e_machineTarget));

		if (std::bit_cast<uint16_t>(readBufferu16) != e_machineTarget) {
			helper::fatalError("ELF header e_machine not valid (" + std::to_string(std::bit_cast<uint16_t>(readBufferu16)) + ")");
		}
	}

	//starting from e_phoff, read the e_phnum program headers of size e_phentsize bytes into ram at p_vaddr
	void parsePH(std::ifstream& fin, MachineCore& machine) {
		std::uint32_t finSize;
		std::uint8_t readBuffer[4];
		std::uint8_t readBufferu16[2];
		std::uint32_t e_phoff;
		std::uint16_t e_phentsize;
		std::uint16_t e_phnum;

		fin.seekg(0, std::ios::end);
		finSize = static_cast<std::uint32_t>(fin.tellg());

		//e_phoff offset = 28, size = 4
		fin.seekg(28, std::ios::beg);
		fin.read(reinterpret_cast<char*>(readBuffer), sizeof(e_phoff));
		e_phoff = std::bit_cast<std::uint32_t>(readBuffer);

		//e_phentsize offset = 42, size = 2
		fin.seekg(42, std::ios::beg);
		fin.read(reinterpret_cast<char*>(readBufferu16), sizeof(e_phentsize));
		e_phentsize = std::bit_cast<std::uint16_t>(readBufferu16);

		//e_phnum offset = 44, size = 2
		fin.read(reinterpret_cast<char*>(readBufferu16), sizeof(e_phnum));
		e_phnum = std::bit_cast<std::uint16_t>(readBufferu16);
		

		if constexpr (std::endian::native == std::endian::big) {
			e_phoff = std::byteswap(e_phoff);
			e_phentsize = std::byteswap(e_phentsize);
			e_phnum = std::byteswap(e_phnum);
		}

		//std::uint16_t stride = e_phentsize / 32;

		for (std::uint16_t ph = 0; ph < e_phnum; ph++) {
			//in order of how they will appear in the PH
			std::uint32_t p_type;
			std::uint32_t p_offset;
			std::uint32_t p_vaddr;
			//p_paddr
			std::uint32_t p_filesz;
			std::uint32_t p_memsz;
			std::uint32_t p_flags;
			std::uint32_t p_align;

			//make sure the space we read exists in file
			if (finSize < e_phoff + (e_phnum + 1) * e_phentsize) {
				helper::fatalError("Not enough space in file to read Program Header (" + std::to_string(finSize) + " < " + std::to_string(e_phoff + (e_phnum + 1) * e_phentsize) + ")");
			}

			//offset for the start of the current program header 
			fin.seekg(e_phoff + ph * e_phentsize, std::ios::beg);
			fin.read(reinterpret_cast<char*>(readBuffer), sizeof(p_type));
			p_type = std::bit_cast<std::uint32_t>(readBuffer);

			//section is not loadable
			if (p_type != p_typeTarget) {
				continue;
			}

			//read the PH and set fields
			fin.read(reinterpret_cast<char*>(readBuffer), sizeof(p_offset));
			p_offset = std::bit_cast<std::uint32_t>(readBuffer);
			fin.read(reinterpret_cast<char*>(readBuffer), sizeof(p_vaddr));
			p_vaddr = std::bit_cast<std::uint32_t>(readBuffer);
			fin.seekg(4, std::ios::cur); //skip p_paddr
			fin.read(reinterpret_cast<char*>(readBuffer), sizeof(p_filesz));
			p_filesz = std::bit_cast<std::uint32_t>(readBuffer);
			fin.read(reinterpret_cast<char*>(readBuffer), sizeof(p_memsz));
			p_memsz = std::bit_cast<std::uint32_t>(readBuffer);
			fin.read(reinterpret_cast<char*>(readBuffer), sizeof(p_flags));
			p_flags = std::bit_cast<std::uint32_t>(readBuffer);
			fin.read(reinterpret_cast<char*>(readBuffer), sizeof(p_align));
			p_align = std::bit_cast<std::uint32_t>(readBuffer);

			//swap all to little endian if machine running this code is big endian
			if constexpr (std::endian::native == std::endian::big) {
				p_offset = std::byteswap(p_offset);
				p_vaddr = std::byteswap(p_vaddr);
				p_filesz = std::byteswap(p_filesz);
				p_memsz = std::byteswap(p_memsz);
				p_flags = std::byteswap(p_flags);
				p_align = std::byteswap(p_align);
			}
			
			//use fields to set machine ram for the current PH
			std::uint32_t programBufferSize = (p_memsz > p_filesz) ? p_memsz : p_filesz;
			std::uint8_t* programBuffer = new std::uint8_t[programBufferSize];

			if (finSize < p_offset + p_filesz) {
				helper::fatalError("Not enough space in file to read Program Header (" + std::to_string(finSize) + " < " + std::to_string(p_offset + p_filesz) + ")");
			}
			fin.seekg(p_offset, std::ios::beg);
			fin.read(reinterpret_cast<char*>(programBuffer), p_filesz);

			//fill remaining buffer with 0's
			if (programBufferSize != p_filesz) {
				std::memset(programBuffer + p_filesz, 0, (p_memsz - p_filesz) * sizeof(std::uint8_t));
			}

			//set the ram at index p_vaddr to programBuffer
			machine.loadRamAtIndex(programBuffer, programBufferSize, p_vaddr);
			delete[] programBuffer;
		}
	}
}

namespace elf {
	//check the EH and make sure its valid
	//then parse the program headers, setting ram on each one (or skipping if not loadable)
	//then return the e_entry point for the PC
	std::uint32_t parseELF(std::string filename, MachineCore& machine) {
		std::ifstream fin(filename, std::ios::binary);
		isValidEH(fin, filename);
		parsePH(fin, machine);

		std::uint32_t e_entry;
		std::uint8_t readBuffer[4];

		//e_entry offset = 24, size = 4
		fin.seekg(24, std::ios::beg);
		fin.read(reinterpret_cast<char*>(readBuffer), sizeof(e_entry));
		e_entry = std::bit_cast<std::uint32_t>(readBuffer);

		if constexpr (std::endian::native == std::endian::big) {
			e_entry = std::byteswap(e_entry);
		}

		fin.close();

		return e_entry;
	}
}