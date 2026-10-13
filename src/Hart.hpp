#pragma once
#include "CSR.hpp"
#include "PC.hpp"
#include "RegisterFile.hpp"
#include <cinttypes>
class Hart
{
public:
	Hart(std::uint32_t hartId, std::uint8_t mmuState);

	//these will possibly be replaced with functions that call RegisterFile/CSR/PC/etc. functions from a Hart
	//for now, we will just return a reference, and the machine will call functions from the reference
	RegisterFile& getRegisterFile();
	CSR& getCSR();
	PC& getPC();
	void print() const;

private:
	uint32_t m_hartId;
	RegisterFile m_registerFile;
	CSR m_CSR;
	PC m_PC;
};

