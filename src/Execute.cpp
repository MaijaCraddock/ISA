#include "Execute.hpp"

#include <limits>

namespace {

	constexpr std::uint8_t kOpR = 0b0110011;
	constexpr std::uint8_t kOpImm = 0b0010011;
	constexpr std::uint8_t kOpStore = 0b0100011;
	constexpr std::uint8_t kOpBranch = 0b1100011;
	constexpr std::uint8_t kOpJalr = 0b1100111;
	constexpr std::uint8_t kOpJal = 0b1101111;
	constexpr std::uint8_t kOpSystem = 0b1110011;

	constexpr std::uint8_t kFunct7Sfence = 0b0001001;
	constexpr std::uint8_t kFunct7MulDiv = 0b0000001;

	//share Sub / Mul AluOps with SUB / MUL
	constexpr std::uint8_t kSltFunct3 = 0b010;
	constexpr std::uint8_t kSltuFunct3 = 0b011;
	constexpr std::uint8_t kMulhFunct3 = 0b001;
	constexpr std::uint8_t kMulhsuFunct3 = 0b010;
	constexpr std::uint8_t kMulhuFunct3 = 0b011;

	constexpr std::uint8_t kBeqFunct3 = 0b000;
	constexpr std::uint8_t kBneFunct3 = 0b001;
	constexpr std::uint8_t kBltFunct3 = 0b100;
	constexpr std::uint8_t kBgeFunct3 = 0b101;
	constexpr std::uint8_t kBltuFunct3 = 0b110;
	constexpr std::uint8_t kBgeuFunct3 = 0b111;

	constexpr std::uint8_t kInstBytes = 4;

	std::uint8_t getOpcode(std::uint32_t inst) { return static_cast<std::uint8_t>(inst & 0b1111111u); }
	std::uint8_t getFunct3(std::uint32_t inst) { return static_cast<std::uint8_t>((inst >> 12) & 0b111u); }
	std::uint8_t getFunct7(std::uint32_t inst) { return static_cast<std::uint8_t>((inst >> 25) & 0b1111111u); }
	std::uint8_t getRs1(std::uint32_t inst) { return static_cast<std::uint8_t>((inst >> 15) & 0b11111u); }
	std::uint8_t getRs2(std::uint32_t inst) { return static_cast<std::uint8_t>((inst >> 20) & 0b11111u); }

	//32bit result + 1 bit nzcv flags
	struct AluOut
	{
		std::uint32_t result;
		std::uint8_t n;
		std::uint8_t z;
		std::uint8_t c;
		std::uint8_t v;
	};

	//called on any non illegal instruction, only checks op
	AluOut alu(AluOp op, std::uint32_t a, std::uint32_t b)
	{
		AluOut out = { 0, 0, 0, 0, 0 }; //result, nzcv
		const std::int32_t sa = static_cast<std::int32_t>(a);
		const std::int32_t sb = static_cast<std::int32_t>(b);
		const std::uint32_t shamt = b & 0x1Fu;

		switch (op) {
		case AluOp::Add:
		case AluOp::Sub: {
			// a + b for ADD, or a + ~b + 1 for SUB
			const std::uint32_t operand = (op == AluOp::Sub) ? ~b : b;
			const std::uint64_t carryIn = (op == AluOp::Sub) ? 1u : 0u;
			const std::uint64_t wide = static_cast<std::uint64_t>(a) + operand + carryIn;
			out.result = static_cast<std::uint32_t>(wide);												//cast wide to 32 bits for result
			out.c = static_cast<std::uint8_t>((wide >> 32) & 1u); 										//extra bit of wide, c == 1 means no borrow
			out.v = static_cast<std::uint8_t>((((~(a ^ operand)) & (a ^ out.result)) >> 31) & 0b1);		//operand = ~b on sub and b on add, checks v correct on both add or sub with same equation
			break;
		}
		case AluOp::Mul:
			out.result = a * b;
			break;
		case AluOp::Div:
			if (b == 0) {
				out.result = 0xFFFFFFFFu;																//divide by zero: x / 0 = -1
			}
			else if (sa == std::numeric_limits<std::int32_t>::min() && sb == -1) {
				out.result = a;																			//overflow: INT_MIN / -1 = INT_MIN
			}
			else {
				out.result = static_cast<std::uint32_t>(sa / sb);
			}
			break;
		case AluOp::DivU:
			out.result = (b == 0) ? 0xFFFFFFFFu : a / b;
			break;
		case AluOp::Rem:
			if (b == 0) {
				out.result = a;																			//divide by zero: x % 0 = x
			}
			else if (sa == std::numeric_limits<std::int32_t>::min() && sb == -1) {
				out.result = 0;																			//overflow: INT_MIN % -1 = 0
			}
			else {
				out.result = static_cast<std::uint32_t>(sa % sb);
			}
			break;
		case AluOp::RemU:
			out.result = (b == 0) ? a : a % b;
			break;
		case AluOp::LeftShift:
			out.result = a << shamt;
			break;
		case AluOp::RightShiftA:
			out.result = static_cast<std::uint32_t>(sa >> shamt);
			break;
		case AluOp::RightShiftL:
			out.result = a >> shamt;
			break;
		case AluOp::Or:
			out.result = a | b;
			break;
		case AluOp::Xor:
			out.result = a ^ b;
			break;
		case AluOp::And:
			out.result = a & b;
			break;
		}

		out.n = static_cast<std::uint8_t>(out.result >> 31);
		out.z = static_cast<std::uint8_t>(out.result == 0);
		return out;
	}

	//check funct3 to differentiate MULH / MULHSU / MULHU
	std::uint32_t multiplyHigh(std::uint8_t funct3, std::uint32_t a, std::uint32_t b)
	{
		const std::int64_t sa = static_cast<std::int32_t>(a);
		const std::int64_t sb = static_cast<std::int32_t>(b);
		const std::uint64_t ua = a;
		const std::uint64_t ub = b;
		std::uint64_t product = 0;

		if (funct3 == kMulhFunct3) {
			product = static_cast<std::uint64_t>(sa * sb);							//signed x signed
		}
		else if (funct3 == kMulhsuFunct3) {
			product = static_cast<std::uint64_t>(sa * static_cast<std::int64_t>(ub));	//signed x unsigned
		}
		else {
			product = ua * ub;														//unsigned x unsigned
		}
		return static_cast<std::uint32_t>(product >> 32);							//upper 32 bits
	}

	//branch condition from the flags of rs1 - rs2 sub aluop
	std::uint8_t branchTaken(std::uint8_t funct3, const AluOut& flags)
	{
		switch (funct3) {
		case kBeqFunct3:	return flags.z;
		case kBneFunct3:	return static_cast<std::uint8_t>(!flags.z);
		case kBltFunct3:	return static_cast<std::uint8_t>(flags.n != flags.v);	//slt
		case kBgeFunct3:	return static_cast<std::uint8_t>(flags.n == flags.v);
		case kBltuFunct3:	return static_cast<std::uint8_t>(!flags.c);				//sltu: a borrow happened
		case kBgeuFunct3:	return flags.c;
		default:			return 0;												//decode already rejected funct3 = 010 and 011
		}
	}

	//LOAD (and AMO): the value only exists after Memory, so it cannot be forwarded
	std::uint8_t isLoadLike(const Instruction& i)
	{
		return (i.memop != 0) && !(i.memop & 0b0100);
	}

	//CSR instructions read and write the CSR in writeback, so their rd value does not exist before then
	std::uint8_t isCsr(std::uint32_t inst)
	{
		return (getOpcode(inst) == kOpSystem) && (getFunct3(inst) != 0);
	}

	std::uint8_t isSfence(std::uint32_t inst)
	{
		return (getOpcode(inst) == kOpSystem) && (getFunct3(inst) == 0) && (getFunct7(inst) == kFunct7Sfence);
	}

	//checks if instruction writes to register
	std::uint8_t writesRegister(const ExecuteIntermediateRegister& r)
	{
		return !r.illegalInst && (r.latch.instr.rd != 0);
	}

	//checks if rs1 is used as a register
	std::uint8_t usesRs1(std::uint32_t inst)
	{
		const InstType type = Decode::getType(inst);
		if (type == InstType::U || type == InstType::J || type == InstType::Illegal) {
			return 0;
		}
		//csrrwi/csrrsi/csrrci use the rs1 field as a 5-bit immediate, not a register
		return !((getOpcode(inst) == kOpSystem) && (getFunct3(inst) >= 0b101));
	}

	std::uint8_t usesRs2(std::uint32_t inst)
	{
		const InstType type = Decode::getType(inst);
		return (type == InstType::R) || (type == InstType::S) || (type == InstType::B) || isSfence(inst);
	}

	//newest value of register reg that is still in flight: EX/MEM beats MEM/WB beats the MEM/WB of a cycle ago, x0 is never forwarded
	std::int32_t forward(std::uint8_t reg, std::int32_t value, const ExecuteIntermediateRegister& exMem, const ExecuteIntermediateRegister& memWb, const ExecuteIntermediateRegister& lastMemWb)
	{
		if (reg == 0) {
			return value;
		}
		if (writesRegister(exMem) && exMem.latch.instr.rd == reg) {
			return exMem.latch.instr.result;
		}
		if (writesRegister(memWb) && memWb.latch.instr.rd == reg) {
			return memWb.latch.instr.result;
		}
		if (writesRegister(lastMemWb) && lastMemWb.latch.instr.rd == reg) {
			return lastMemWb.latch.instr.result;
		}
		return value;
	}

	//checks if the value of a register is ready yet, prioritizes most recent writer (inExecute > inMemory > inWriteback)
	std::uint8_t mustWait(std::uint8_t reg, const ExecuteIntermediateRegister& inExecute, const ExecuteIntermediateRegister& inMemory, const ExecuteIntermediateRegister& inWriteback)
	{
		if (reg == 0) {
			return 0;
		}
		if (writesRegister(inExecute) && inExecute.latch.instr.rd == reg) {
			//A LOAD (and memop) does not have its value until after the memory stage, A CSR does not have its value until after the writeback stage
			return isLoadLike(inExecute.latch.instr) || isCsr(inExecute.latch.instr.inst);
		}
		if (writesRegister(inMemory) && inMemory.latch.instr.rd == reg) {
			return isCsr(inMemory.latch.instr.inst);
		}
		if (writesRegister(inWriteback) && inWriteback.latch.instr.rd == reg) {
			return isCsr(inWriteback.latch.instr.inst);
		}
		return 0;
	}
}

Execute::Execute(Fetch& fetch, Decode& decode, const ExecuteIntermediateRegister& memWbLatch)
	: m_fetch(fetch)
	, m_decode(decode)
	, m_memWbLatch(memWbLatch)
	, m_current(bubble())
	, m_next(bubble())
	, m_lastMemWb(bubble())
	, m_nextLastMemWb(bubble())
{
}

const ExecuteIntermediateRegister& Execute::outputLatch() const
{
	return m_current;
}

ExecuteIntermediateRegister Execute::bubble()
{
	return Decode::bubble();
}

void Execute::executeInst()
{
	Instruction& d = m_next.latch.instr;
	const std::uint32_t inst = d.inst;
	const std::uint32_t pc = m_next.latch.currPC;
	const std::uint8_t opcode = getOpcode(inst);
	const std::uint8_t funct3 = getFunct3(inst);
	const std::uint8_t funct7 = getFunct7(inst);

	//operand forwarding 
	//Reads only the current latches: EX/MEM is m_current, MEM/WB is the Memory's output, m_lastMemWb is what MEM/WB held last cycle
	if (usesRs1(inst)) {
		d.left = forward(getRs1(inst), d.left, m_current, m_memWbLatch, m_lastMemWb);
	}
	if (usesRs2(inst)) {
		if (opcode == kOpStore) {
			d.disp_strval = forward(getRs2(inst), d.disp_strval, m_current, m_memWbLatch, m_lastMemWb);	//the value to store
		}
		else {
			d.right = forward(getRs2(inst), d.right, m_current, m_memWbLatch, m_lastMemWb);
		}
	}

	//the ALU only looks at aluop, left and right. LOAD/STORE/AUIPC/LUI come out as adds
	const AluOut out = alu(static_cast<AluOp>(d.aluop), static_cast<std::uint32_t>(d.left), static_cast<std::uint32_t>(d.right));
	std::uint32_t result = out.result;

	std::uint8_t taken = 0;
	std::uint32_t target = 0;

	//differentiate instructions with the same alu op (mul) and different funct3's
	if (opcode == kOpR && funct7 == kFunct7MulDiv && (funct3 == kMulhFunct3 || funct3 == kMulhsuFunct3 || funct3 == kMulhuFunct3)) {
		result = multiplyHigh(funct3, static_cast<std::uint32_t>(d.left), static_cast<std::uint32_t>(d.right));
	}
	else if (((opcode == kOpR && funct7 == 0) || opcode == kOpImm) && (funct3 == kSltFunct3 || funct3 == kSltuFunct3)) {
		//SLT / SLTU: the result is the condition bit (bit 0)
		result = (funct3 == kSltFunct3) ? static_cast<std::uint32_t>(out.n != out.v) : static_cast<std::uint32_t>(!out.c);
	}
	else if (opcode == kOpBranch) {
		taken = branchTaken(funct3, out);
		target = pc + static_cast<std::uint32_t>(d.disp_strval);
		result = 0;
	}
	else if (opcode == kOpJal) {
		taken = 1;
		target = pc + static_cast<std::uint32_t>(d.disp_strval);
		result = pc + kInstBytes;			//the link value goes to rd
	}
	else if (opcode == kOpJalr) {
		taken = 1;
		target = out.result & ~1u;			//rs1 + imm, bit 0 cleared
		result = pc + kInstBytes;
	}

	//branch resolution: compare the real next PC with the one fetch predicted
	if (opcode == kOpBranch || opcode == kOpJal || opcode == kOpJalr) {
		const std::uint32_t actualNextPC = taken ? target : pc + kInstBytes;

		if (opcode == kOpBranch) {
			m_fetch.recordBranchOutcome(pc, taken);
		}
		if (actualNextPC != m_next.latch.predictPC) {
			//misprediction: bubble and redirect the PC
			m_fetch.requestRedirect(actualNextPC);
			m_decode.requestBubble();
		}
	}

	d.result = static_cast<std::int32_t>(result);
}

std::uint8_t Execute::needsStall() const
{
	const std::uint32_t inst = m_fetch.outputLatch().instr.inst;		//== instruction in decode right now
	const std::uint8_t rs1 = usesRs1(inst) ? getRs1(inst) : 0;
	const std::uint8_t rs2 = usesRs2(inst) ? getRs2(inst) : 0;

	//load-use: check both rs1 and rs2 to see if a bubble is neccesary, LOAD checks memory and CSR inst checks up to writeback
	const ExecuteIntermediateRegister& inExecute = m_decode.outputLatch();
	//parameters: rs1/2, execute instruction, memory instruction, writeback instruction
	return mustWait(rs1, inExecute, m_current, m_memWbLatch) || mustWait(rs2, inExecute, m_current, m_memWbLatch);
}

void Execute::rising_edge()
{
	m_next = m_decode.outputLatch();
	m_nextLastMemWb = m_memWbLatch;				//save for forwarding (do not use until next rising edge)

	if (!m_next.illegalInst) {
		executeInst();
	}

	if (needsStall()) {
		m_fetch.requestStall();					// freeze pc and latch in fetch
		m_decode.requestBubble();				// request bubble in decode, which sends a bubble to execute
	}
}

void Execute::falling_edge()
{
	m_current = m_next;
	m_lastMemWb = m_nextLastMemWb;				//save for forwarding (is now set for next rising edge)
}