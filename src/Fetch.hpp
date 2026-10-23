#pragma once
#include <cstdint>
#include <vector>
#include "IMemorySeam.hpp"
#include "BranchPredictor.hpp"
class Hart; // circular dependancy with #include "Hart.hpp"

struct Instruction 
{
	std::uint32_t inst = 0;
	std::int32_t left = 0;
	std::int32_t right = 0;
	std::int32_t result = 0;
	std::int32_t disp_strval = 0;
	std::uint8_t rd = 0;
	std::uint8_t memop = 0;
	std::uint8_t aluop = 0;
};

struct FetchIntermediateRegister 
{
	Instruction instr;
	std::uint32_t currPC = 0;
	std::uint32_t predictPC = 0;
	std::uint8_t  bpTaken = 0; //bool checks if branch predict was taken
};

class Fetch
{
public:
	static constexpr std::uint32_t kNOP = 0x00000013u; //addi zero, zero, 0

	Fetch(Hart& hart, IMemorySeam& memory, std::uint32_t bpRows, std::uint8_t bpDefaultState);
	void start();

	void rising_edge();
	void falling_edge();

	const FetchIntermediateRegister& outputLatch() const;

	void requestStall();
	void requestRedirect(std::uint32_t target);
	void recordBranchOutcome(std::uint32_t branchPC, std::uint8_t taken);

	const BranchPredictor& predictor() const;

	static FetchIntermediateRegister bubble();

private:
	static constexpr std::uint8_t  kInstructionBytes = 4;
	static constexpr std::uint32_t kOpcodeMask = 0x7Fu;
	static constexpr std::uint32_t kOpcodeBranch = 0x63u;

	enum class State : std::uint8_t {Idle, Waiting, StartWait, Deliver, DeliverHeld, Fault};

	struct NextLatch
	{
		State state = State::Idle;
		FetchIntermediateRegister out;
		std::uint32_t inst = 0;
		std::uint32_t pc = 0;
		std::uint32_t latency = 0;
		MemStatus fault = MemStatus::Ok;
	};

	struct PendingUpdate
	{
		std::uint8_t valid = 0;
		std::uint32_t pc = 0;
		std::uint8_t taken = 0;
	};

	std::int32_t branchDisplacement(std::uint32_t inst);
	void swapLatch(std::uint32_t inst, std::uint32_t pc, State state);

	Hart& m_hart;
	IMemorySeam& m_memory;
	BranchPredictor m_predictor;
	FetchIntermediateRegister m_current;
	NextLatch m_next;

	std::uint8_t m_hasHeld = 0;
	std::uint32_t m_heldInst = 0;
	std::uint32_t m_heldPC = 0;
	std::uint32_t m_waitLeft = 0;

	std::uint8_t m_stallRequested = 0;
	std::uint8_t m_redirectRequested = 0;
	std::uint32_t m_redirectTarget = 0;
	PendingUpdate m_pendingUpdate;
};