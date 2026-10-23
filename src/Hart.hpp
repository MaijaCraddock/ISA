#pragma once
#include "CSR.hpp"
#include "PC.hpp"
#include "RAM.hpp"
#include "RegisterFile.hpp"
#include "IMemorySeam.hpp"
#include "Fetch.hpp"
#include "Decode.hpp"

#include <memory>
#include <cinttypes>

enum class HartStatus : std::int32_t { Running, Stopped, StorePageFault, LoadPageFault, FetchPageFault, IllegalInstruction, AccessFault };

class Hart
{
public:
	Hart(std::uint32_t hartId, std::uint8_t mmuState, RAM& ram, std::uint32_t bpRows, std::uint8_t bpDefault, std::uint8_t extM, std::uint8_t extA);
	//disable copying
	Hart(const Hart&) = delete;
	Hart& operator=(const Hart&) = delete;

	//these will possibly be replaced with functions that call RegisterFile/CSR/PC/etc. functions from a Hart
	//for now, we will just return a reference, and the machine will call functions from the reference
	RegisterFile& getRegisterFile();
	CSR& getCSR();
	PC& getPC();
	void print() const;

	HartStatus getStatus() const;
	void park(HartStatus s);

	void start();
	void rising_edge();
	void falling_edge();
private:
	uint32_t m_hartId;
	HartStatus m_status;
	RegisterFile m_registerFile;
	CSR m_CSR;
	PC m_PC;
	IMemorySeam& m_memorySeam;
	Fetch m_Fetch;
	Decode m_Decode;
};

