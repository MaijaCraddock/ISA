#include "Fetch.hpp"
#include "Hart.hpp"
#include "SignExtend.hpp"

Fetch::Fetch(Hart& hart, IMemorySeam& memory, std::uint32_t bpRows, std::uint8_t bpDefaultState)
	: m_hart(hart)
	, m_memory(memory)
	, m_predictor(bpRows, bpDefaultState)
	, m_current(bubble()) //bubble the first operation so future steps are in parallel
{
}

//first fetch of the HART
void Fetch::start()
{
	m_current = bubble();
	m_next = NextLatch{};
	m_hasHeld = 0;
	m_waitLeft = 0;
	m_stallRequested = 0;
	m_redirectRequested = 0;
	m_pendingUpdate = PendingUpdate{};
}


const FetchIntermediateRegister& Fetch::outputLatch() const
{
	return m_current;
}

void Fetch::requestStall()
{
	m_stallRequested = 1;
}

void Fetch::requestRedirect(std::uint32_t target)
{
	m_redirectRequested = 1;
	m_redirectTarget = target;
}
void Fetch::recordBranchOutcome(std::uint32_t branchPC, std::uint8_t taken)
{
	m_pendingUpdate.valid = 1;
	m_pendingUpdate.pc = branchPC;
	m_pendingUpdate.taken = taken;
}

const BranchPredictor& Fetch::predictor() const
{
	return m_predictor;
}

FetchIntermediateRegister Fetch::bubble()
{
	FetchIntermediateRegister nop;
	nop.instr.inst = kNOP;
	return nop;
}

std::int32_t Fetch::branchDisplacement(std::uint32_t inst)
{
	const std::uint32_t imm =
		(((inst >> 31) & 0x01u) << 12) |
		(((inst >> 7) & 0x01u) << 11) |
		(((inst >> 25) & 0x3Fu) << 5) |
		(((inst >> 8) & 0x0Fu) << 1);
	return SignExtend::signExtend(static_cast<std::int32_t>(imm), 12);
}


//set front buffer to back buffer
void Fetch::swapLatch(std::uint32_t inst, std::uint32_t pc, State state)
{
	FetchIntermediateRegister out;
	out.instr.inst = inst; 
	out.currPC = pc;

	std::uint8_t taken = 0;
	std::uint32_t next = pc + kInstructionBytes;

	if ((inst & kOpcodeMask) == kOpcodeBranch) {
		taken = m_predictor.predictTaken(pc);
		if (taken) {
			next = pc + static_cast<std::uint32_t>(branchDisplacement(inst));
		}
	}
	out.bpTaken = taken;
	out.predictPC = next;

	m_next.state = state;
	m_next.out = out;
}

void Fetch::rising_edge()
{
	m_next = NextLatch{};
	m_next.out = bubble();

	// parked HART
	if (m_hart.getStatus() != HartStatus::Running) {
		return;   // idle
	}

	// multi-cycle fetch in progress
	if (m_hasHeld) {
		if (m_waitLeft > 1) {
			m_next.state = State::Waiting;
			return;
		}
		swapLatch(m_heldInst, m_heldPC, State::DeliverHeld);
		return;
	}

	const std::uint32_t pc = m_hart.getPC().getPC();
	const MemResult r = m_memory.read(pc, kInstructionBytes, AccessType::Fetch);

	//memory fault
	if (r.status != MemStatus::Ok) {
		m_next.state = State::Fault;
		m_next.fault = r.status;
		return;
	}
	
	if (r.latency > 1) {
		m_next.state = State::StartWait;
		m_next.inst = r.value;
		m_next.pc = pc;
		m_next.latency = r.latency;
		return;
	}

	swapLatch(r.value, pc, State::Deliver);
}

void Fetch::falling_edge()
{
	//check pending update
	if (m_pendingUpdate.valid) {
		m_predictor.update(m_pendingUpdate.pc, m_pendingUpdate.taken);
		m_pendingUpdate.valid = false;
	}

	const std::uint8_t deliveredThisCycle = (m_next.state == State::Deliver) || (m_next.state == State::DeliverHeld);

	//if there was a redirect, bubble the current latch
	if (m_redirectRequested) {
		m_current = bubble();
		m_hasHeld = false;
		m_waitLeft = 0;
		m_hart.getPC().setPC(m_redirectTarget);
		if (deliveredThisCycle) {
			m_hart.getCSR().incInstret();
		}
		m_redirectRequested = false;
		m_stallRequested = false;
		return;
	}

	//if there was a stall, stall the fetch and skip CSR incInstret()
	if (m_stallRequested) {
		m_stallRequested = false;
		return;
	}

	// commit the next latch 
	if (deliveredThisCycle) {
		//set the current latch to the next one (for decode in next rising edge)
		m_current = m_next.out;
		m_hart.getPC().setPC(m_next.out.predictPC);   // PC += 4
		m_hart.getCSR().incInstret();
		m_hasHeld = false;
	}

	else if (m_next.state == State::StartWait) {
		// bubble the current latch, set held values for future
		m_current = bubble();
		m_hasHeld = true;
		m_heldInst = m_next.inst;
		m_heldPC = m_next.pc;
		m_waitLeft = m_next.latency - 1;
	}

	else if (m_next.state == State::Waiting) {
		// bubble the current latch, dec counter
		m_current = bubble();
		m_waitLeft -= 1;
	}

	else if (m_next.state == State::Fault) {
		// 
		m_hart.park(m_next.fault == MemStatus::AccessFault ? HartStatus::AccessFault : HartStatus::FetchPageFault);
		m_current = bubble();
	}

	else {
		m_current = bubble();   // idle
	}
}