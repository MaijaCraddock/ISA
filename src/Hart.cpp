#include "Hart.hpp"

//MUST use member intializer list, CSR has no defualt constructor
Hart::Hart(std::uint32_t hartId, std::uint8_t mmuState)
	: m_hartId			{ hartId}
	, m_registerFile	{ RegisterFile()}
	, m_CSR				{ CSR(hartId, mmuState) }
	, m_PC				{ PC()}
{
}

RegisterFile& Hart::getRegisterFile()
{
	return m_registerFile;
}

CSR& Hart::getCSR()
{
	return m_CSR;
}

PC& Hart::getPC()
{
	return m_PC;
}

void Hart::print() const{
	printf(	"	Hart ID: %" PRIu32 "\n", m_hartId);
	printf("	Register File - 32bit Integer Register\n");
	for (std::uint8_t rfIdx = 0; rfIdx < 32; rfIdx++) {
		printf("	* Register %" PRIu8 ": %" PRId32 "\n", rfIdx, m_registerFile.read(rfIdx));
	}

	constexpr uint16_t k_mhartid = 0xF14;
	constexpr uint16_t k_satp = 0x180;
	constexpr uint16_t k_time = 0xC01;
	constexpr uint16_t k_timeh = 0xC81;
	constexpr uint16_t k_cycle = 0xC00;
	constexpr uint16_t k_cycleh = 0xC80;
	constexpr uint16_t k_instret = 0xC02;
	constexpr uint16_t k_instreth = 0xC82;
	
	printf("	CSR Registers\n");
	printf("	* Register (mhartid)   %" PRIu16 ": %" PRIu32 "\n",	k_mhartid,	m_CSR.read(k_mhartid));
	printf("	* Register (msatp)     %" PRIu16 ": %" PRIu32 "\n",	k_satp ,	m_CSR.read(k_satp));
	printf("	* Register (mtime)     %" PRIu16 ": %" PRIu32 "\n",	k_time ,	m_CSR.read(k_time));
	printf("	* Register (mtimeh)    %" PRIu16 ": %" PRIu32 "\n",	k_timeh ,	m_CSR.read(k_timeh));
	printf("	* Register (mcycle)    %" PRIu16 ": %" PRIu32 "\n",	k_cycle ,	m_CSR.read(k_cycle));
	printf("	* Register (mcycleh)   %" PRIu16 ": %" PRIu32 "\n",	k_cycleh,	m_CSR.read(k_cycleh));
	printf("	* Register (minstret)  %" PRIu16 ": %" PRIu32 "\n",	k_instret,	m_CSR.read(k_instret));
	printf("	* Register (minstreth) %" PRIu16 ": %" PRIu32 "\n", k_instreth,	m_CSR.read(k_instreth));

	printf("	Program Counter: %" PRIu32 "\n", m_PC.getPC());
}