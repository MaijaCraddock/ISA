#include "BranchPredictor.hpp"

BranchPredictor::BranchPredictor(std::uint32_t rows, std::uint8_t defaultState)
	: m_rows(rows)
	, m_defaultState(static_cast<std::uint8_t>(defaultState & 0x3u))
	, m_table(rows)
{
}

std::uint8_t BranchPredictor::predictTaken(std::uint32_t branchPC)
{
	if (m_rows == 0) { //not enabled
		return 0;
	}
	// shift and see if msb is set, if it is, the predict is taken, if not, it is not taken
	return static_cast<bool>((findOrAlloc(branchPC).history >> 1) & 0x1u);
}

void BranchPredictor::update(std::uint32_t branchPC, std::uint8_t taken)
{
	if (m_rows == 0) { //not enabled
		return;
	}

	Entry& e = findOrAlloc(branchPC);
	
	// shift new value into lsb, then mask with 0b11
	const std::uint32_t shifted = (static_cast<std::uint32_t>(e.history) << 1) | static_cast<std::uint32_t>(taken);
	e.history = static_cast<std::uint8_t>(shifted & 0x3u);
}

std::uint8_t BranchPredictor::lookup(std::uint32_t branchPC, std::uint8_t& history) const
{
	if (m_rows == 0) { //not enabled
		return 0;
	}

	const Entry& e = m_table[indexPC(branchPC)];
	if (!e.valid || e.pc != branchPC) {
		return 0;
	}

	history = e.history;
	return 1;
}

//shift the PC so that the value returned is a usable index
std::uint32_t BranchPredictor::indexPC(std::uint32_t branchPC) const
{
	return (branchPC >> 2) % m_rows;
}

//find the entry at branchPC or allocate it on a miss
BranchPredictor::Entry& BranchPredictor::findOrAlloc(std::uint32_t branchPC)
{
	Entry& e = m_table[indexPC(branchPC)];
	if (!e.valid || e.pc != branchPC) {
		// Compulsory miss / conflict miss 
		// use -bpD
		e.pc = branchPC;
		e.history = m_defaultState;
		e.valid = 1;
	}
	return e;
}

