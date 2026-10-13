#include "CSR.hpp"
#include "HelperFunctions.hpp"

std::uint32_t CSR::read(std::uint16_t CSRNumber) const
{
	switch (CSRNumber) {
	case k_mhartid:
		return CSR::readmhartid();
	case k_satp:
		return CSR::readsatp();
	case k_time:
		return CSR::readtime();
	case k_timeh:
		return CSR::readtimeh();
	case k_cycle:
		return CSR::readcycle();
	case k_cycleh:
		return CSR::readcycleh();
	case k_instret:
		return CSR::readinstret();
	case k_instreth:
		return CSR::readinstreth();
	default:
		//fatalError is called here, as we should not even be in this function without one of these CSRs
		helper::fatalError("CSR:read on illegal CSR number (" + std::to_string(CSRNumber) + ")");
	}
}

//returns if write is successful (1) or if hart should park (0) from write to RO CSR as bool/uint8_t
std::uint8_t CSR::write(std::uint16_t CSRNumber, std::uint32_t CSRValue)
{
	switch (CSRNumber) {
	//cases for RW CSRs (only satp)
	case k_satp:
		//even if mmu is off, we dont treat this as a write on RO error
		//instead we just ignore the write, and satp is hardwired to 0
		CSR::writesatp(CSRValue);
		return 1;

	//cases for RO CSRs
	case k_mhartid:
	case k_time:
	case k_timeh:
	case k_cycle:
	case k_cycleh:
	case k_instret:
	case k_instreth:
		return 0;
	
	default:
		//fatalError is called here, as we should not even be in this function without one of these CSRs
		helper::fatalError("CSR:write on illegal CSR number (" + std::to_string(CSRNumber) + ")");
	}
}

void CSR::incCycle() {
	CSR::m_cycle++;
}

void CSR::incInstret() {
	CSR::m_instret++;
}

CSR::CSR(std::uint32_t mhartid, std::uint8_t mmuState)
{
	CSR::m_mhartid = mhartid;
	CSR::m_mmuState = mmuState;
	CSR::m_satp    = 0;
	CSR::m_cycle   = 0;
	CSR::m_instret = 0;
}

std::uint32_t CSR::readmhartid() const
{
	return CSR::m_mhartid;
}
std::uint32_t CSR::readsatp() const 
{
	return CSR::m_satp;
}

//note: if we want to NOT miss any time between calling instructions readtime() and readtimeh(),
//then store the variable "now" in each of these as a member variable, and only update once both finish
//for now, I assume the logic error is intentional

std::uint32_t CSR::readtime() const
{
	//Reference: https://en.cppreference.com/cpp/chrono/time_point/time_since_epoch
	//now is set to the system_clock's current time at "now"
	const std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	//time is set to the number of seconds between now and clock's epoch (Thu Jan  1 00:00:00 1970)
	const std::chrono::seconds time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
	
	//.count() returns the number of ticks for this duration, the mask returns the lower 32 bits
	return (time.count() & 0xFFFFFFFF);
}

std::uint32_t CSR::readtimeh() const
{
	//Reference: https://en.cppreference.com/cpp/chrono/time_point/time_since_epoch
	//now is set to the system_clock's current time at "now"
	const std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	//time is set to the number of seconds between now and clock's epoch (Thu Jan  1 00:00:00 1970)
	const std::chrono::seconds time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());

	//.count() returns the number of ticks for this duration, right shift returns the upper 32 bits
	return (time.count() >> 32);
}
std::uint32_t CSR::readcycle() const
{
	return (CSR::m_cycle & 0xFFFFFFFF);
}
std::uint32_t CSR::readcycleh() const
{
	return (CSR::m_cycle >> 32);
}
std::uint32_t CSR::readinstret() const
{
	return (CSR::m_instret & 0xFFFFFFFF);
}
std::uint32_t CSR::readinstreth() const
{
	return (CSR::m_instret >> 32);
}

void CSR::writesatp(uint32_t CSRValue) {
	//OnOff::Off == 0
	if (m_mmuState != 0) {
		CSR::m_satp = CSRValue;
	}
}