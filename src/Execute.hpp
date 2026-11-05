#pragma once
#include <cstdint>
#include "Fetch.hpp"
#include "Decode.hpp"

using ExecuteIntermediateRegister = DecodeIntermediateRegister;

class Execute
{
public:
	//memWbLatch is the Memory output latch (the instruction currently in writeback)
	Execute(Fetch& fetch, Decode& decode, const ExecuteIntermediateRegister& memWbLatch);

	void rising_edge();
	void falling_edge();

	const ExecuteIntermediateRegister& outputLatch() const;

	static ExecuteIntermediateRegister bubble();

private:
	//needs more than just the output latch, as we also need to send signals to the previous stages
	Fetch& m_fetch;
	Decode& m_decode;
	const ExecuteIntermediateRegister& m_memWbLatch;

	ExecuteIntermediateRegister m_current;	//front buffer
	ExecuteIntermediateRegister m_next;		//back buffer

	//hold Memory's previous output latch for forwarding computations
	//nextLastMemWb is set on a rising edge, and sets the lastMemWb the value it holds on a falling edge, 
	//so that forwarding works on the current rising edge and the next rising edge
	ExecuteIntermediateRegister m_lastMemWb;
	ExecuteIntermediateRegister m_nextLastMemWb;

	void executeInst();
	//true when the instruction in decode has to wait one more cycle (load-use hazard)
	std::uint8_t needsStall() const;
};