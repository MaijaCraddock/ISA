#include "Decode.hpp"
#include "SignExtend.hpp"

namespace 
{
	constexpr std::uint8_t kOpMask   =  0b1111111u;
	//0b inst[6:2] 0b11 (all opcodes end in 0b11)  - What Can Call These -					- Type -
	constexpr std::uint8_t kOpLoad	 =	0b0000011u;	//base									   I
	constexpr std::uint8_t kOpStore  =	0b0100011u;	//base									   S
	constexpr std::uint8_t kOpBranch =	0b1100011u;	//base									   B
	constexpr std::uint8_t kOpJalr   =	0b1100111u;	//base									   I
	constexpr std::uint8_t kOpJal	 =  0b1101111u;	//base									   J
	constexpr std::uint8_t kOpImm	 =  0b0010011u;	//base									   I
	constexpr std::uint8_t kOpR		 =	0b0110011u;	//base, m_extM							   R
	constexpr std::uint8_t kOpSystem =	0b1110011u;	//		Zicsr, Zicntr, SFENCE.VMA		   I
	constexpr std::uint8_t kOpAuipc  =	0b0010111u;	//base									   U
	constexpr std::uint8_t kOpLui	 =	0b0110111u;	//base									   U
	constexpr std::uint8_t kOpAmo	 =  0b0101111u;	//		m_extA							   R

	//AluOp
	constexpr std::uint8_t kAluOpFunct7		= 0b0000000u; //used by all alu ops that arent SRA, SUB or a extM instruction
	constexpr std::uint8_t kSra_SubFunct7	= 0b0100000u; //used only by SRA and SUB
	constexpr std::uint8_t kAdd_SubFunct3	= 0b000u;
	constexpr std::uint8_t kSllFunct3		= 0b001u;
	constexpr std::uint8_t kSltFunct3		= 0b010u;
	constexpr std::uint8_t kSltuFunct3		= 0b011u;
	constexpr std::uint8_t kSrFunct3		= 0b101u;
	constexpr std::uint8_t kOrFunct3		= 0b110u;
	constexpr std::uint8_t kXorFunct3		= 0b100u;
	constexpr std::uint8_t kAndFunct3		= 0b111u;

	//SFENCE.VMA	(Note: R-Type)
	constexpr std::uint8_t kSFenceFunct7 = 0b0001001u;
	//constexpr std::uint8_t kSFenceFunct3 = 0b000u; - we check this in loop prior to checking other fence fields
	constexpr std::uint8_t kSFenceRd	 = 0b00000u;

	//m_extM		(Note: R-Type) - Also AluOps, but only legal when m_extM == 1
	constexpr std::uint8_t kMulDivFunct7	= 0b0000001u;
	constexpr std::uint8_t kMulFunct3		= 0b000u;
	constexpr std::uint8_t kMulhFunct3		= 0b001u;
	constexpr std::uint8_t kMulhsuFunct3	= 0b010u;
	constexpr std::uint8_t kMulhuFunct3		= 0b011u;
	constexpr std::uint8_t kDivFunct3		= 0b100u;
	constexpr std::uint8_t kDivuFunct3		= 0b101u;
	constexpr std::uint8_t kRemFunct3		= 0b110u;
	constexpr std::uint8_t kRemuFunct3		= 0b111u;

	//Zalrsc	(m_extA)
	constexpr std::uint8_t kAmoFunct3		= 0b010u;
	constexpr std::uint8_t kLRFunct5		= 0b00010u;
	constexpr std::uint8_t kSCFunct5		= 0b00011u;

	//Zaamo		(m_extA)
	constexpr std::uint8_t kAmoSwapFunct5	= 0b00001u;
	constexpr std::uint8_t kAmoAddFunct5	= 0b00000u;
	constexpr std::uint8_t kAmoXorFunct5	= 0b00100u;
	constexpr std::uint8_t kAmoAndFunct5	= 0b01100u;
	constexpr std::uint8_t kAmoOrFunct5		= 0b01000u;
	constexpr std::uint8_t kAmoMinuFunct5	= 0b11000u;
	constexpr std::uint8_t kAmoMaxuFunct5	= 0b11100u;

	//Zicsr - for now we dont need to differentiate, but we might need these in a later stage
	/*
	constexpr std::uint8_t kCsrrwFunct3		= 0b001u;
	constexpr std::uint8_t kCsrrsFunct3		= 0b010u; //funct3 for Zicntr
	constexpr std::uint8_t kCsrrcFunct3		= 0b011u;
	constexpr std::uint8_t kCsrrwiFunct3	= 0b101u;
	constexpr std::uint8_t kCsrrsiFunct3	= 0b110u;
	constexpr std::uint8_t kCsrrciFunct3	= 0b111u;
	*/

	//Zicntr's CSR is just the 12 bit CSR stored as a uint16_t in CSR.hpp
	
	//https://web.eecs.utk.edu/~smarz1/docs/riscv-spec.html#rv32
	//R-Type	:		funct7		| rs2	| rs1 | funct3	|	   	  rd	   | opcode
	//I-Type	:			imm[11:0]		| rs1 | funct3	|	   	  rd	   | opcode 
	//S-Type	:	   imm[11:5]	| rs2	| rs1 | funct3	|      imm[4:0]    | opcode
	//B-Type	: [12] | imm[10:5]  | rs2	| rs1 | funct3	| imm[4:1] |  [11] | opcode
	//U-Type	:				imm[31:12]					|	   	  rd	   | opcode 
	//J-Type	: [20] | imm[10:1]  | [11]	| imm[19:12]	|	   	  rd	   | opcode 
	//AMO		: funct5 | aq | rl  | rs2	| rs1 | funct3  |		  rd	   | opcode (kOpAmo)
	//CSR		:			csr				| rs1 | funct3	|		  rd	   | opcode (kOpSystem)
	std::uint8_t getRd(std::uint32_t inst)		{ return static_cast<std::uint8_t>((inst >> 7)  & 0b11111u);	}
	std::uint8_t getFunct3(std::uint32_t inst)	{ return static_cast<std::uint8_t>((inst >> 12) & 0b111u);		}
	std::uint8_t getRs1(std::uint32_t inst)		{ return static_cast<std::uint8_t>((inst >> 15) & 0b11111u);	}
	std::uint8_t getRs2(std::uint32_t inst)		{ return static_cast<std::uint8_t>((inst >> 20) & 0b11111u);	}
	std::uint8_t getFunct7(std::uint32_t inst)	{ return static_cast<std::uint8_t>((inst >> 25) & 0b1111111u);	}
	
	std::uint8_t getFunct5(std::uint32_t inst)	{ return static_cast<std::uint8_t>((inst >> 27) & 0b11111u);	}
	std::uint8_t getAq(std::uint32_t inst)		{ return static_cast<std::uint8_t>((inst >> 26) & 0b1u);		}
	std::uint8_t getRl(std::uint32_t inst)		{ return static_cast<std::uint8_t>((inst >> 25) & 0b1u);		}

	std::int32_t getCSR(std::uint32_t inst)		{ return static_cast<std::int32_t>((inst >> 20) & 0xFFF);		}

	std::int32_t getImmIType(std::uint32_t inst)
	{
		const std::uint32_t imm = (inst >> 20)	& 0xFFFu;	// [11:0]
		return SignExtend::signExtend(static_cast<std::int32_t>(imm), 11);
	}

	std::int32_t getImmSType(std::uint32_t inst)
	{
		const std::uint32_t imm = (inst >> 7 )	& 0x1Fu		// [4:0]
								| (inst >> 20)	& 0xFE0u;	// [11:5]
		return SignExtend::signExtend(static_cast<std::int32_t>(imm), 11);
	}

	std::int32_t getImmBType(std::uint32_t inst)
	{
		const std::uint32_t imm = (inst >> 7 )	& 0x1Eu		// [4:1]
								| (inst >> 20)	& 0x7E0u	// [10:5]
								| (inst << 4 )	& 0x800u	// [11]
								| (inst >> 19)	& 0x1000u;	// [12]
		return SignExtend::signExtend(static_cast<std::int32_t>(imm), 12);
	}

	std::int32_t getImmUType(std::uint32_t inst)
	{
		const std::uint32_t imm = inst & 0xFFFFF000u;		// [31:12]
		return SignExtend::signExtend(static_cast<std::int32_t>(imm), 31);
	}

	std::int32_t getImmJType(std::uint32_t inst)
	{
		const std::uint32_t imm = (inst >> 20)	& 0x7FEu		// [10:1]
								| (inst >> 9 )	& 0x800u		// [11]
								|  inst			& 0xFF000u		// [19:12]
								| (inst >> 11)	& 0x100000u;	// [20]
		return SignExtend::signExtend(static_cast<std::int32_t>(imm), 20);
	}
}

Decode::Decode(const RegisterFile& registerFile, const FetchIntermediateRegister& fetchLatch, std::uint8_t extM, std::uint8_t extA)
	: m_registerFile(registerFile)
	, m_fetchLatch(fetchLatch)
	, m_extM(extM)
	, m_extA(extA)
	, m_current(bubble())
	, m_next(bubble())
{
}

void Decode::rising_edge()
{
	//decode on rising edge
	m_next = decodeInst();
}

void Decode::falling_edge()
{
	//right on falling edge
	m_current = m_bubbleRequested ? bubble() : m_next;
	m_bubbleRequested = 0;
}

const DecodeIntermediateRegister& Decode::outputLatch() const
{
	return m_current;
}

void Decode::requestBubble()
{
	m_bubbleRequested = 1;
}

DecodeIntermediateRegister Decode::bubble()
{
	DecodeIntermediateRegister nop;
	nop.latch = Fetch::bubble();
	return nop;
}

InstType Decode::getType(std::uint32_t inst)
{
	if ((inst & 0b11u) != 0b11u) {
		return InstType::Illegal;	//[1:0] = 0b11 always
	}

	switch (inst & kOpMask) {
	case kOpR:
	case kOpAmo:
		return InstType::R;
	case kOpImm:
	case kOpLoad:
	case kOpJalr:
	case kOpSystem:
		return InstType::I;
	case kOpStore:
		return InstType::S;
	case kOpBranch:
		return InstType::B;
	case kOpLui:
	case kOpAuipc:
		return InstType::U;
	case kOpJal:
		return InstType::J;
	default:
		return InstType::Illegal;
	}
}

DecodeIntermediateRegister Decode::decodeInst() const
{
	DecodeIntermediateRegister out;
	out.latch = m_fetchLatch;
	const std::uint32_t inst = m_fetchLatch.instr.inst;

	Instruction& d = out.latch.instr;
	d = Instruction{};
	d.inst = inst;

	std::uint8_t legal = 0;
	if (getType(inst) != InstType::Illegal) { //keep legal = 0 if Illegal type
		//mask opcode, decode based on opcode, and set legal to decode output
		switch (inst & kOpMask) {
		case kOpR:      legal = decodeReg(inst, d);    break;
		case kOpImm:    legal = decodeImm(inst, d);    break;
		case kOpLoad:   legal = decodeLoad(inst, d);   break;
		case kOpStore:  legal = decodeStore(inst, d);  break;
		case kOpBranch: legal = decodeBranch(inst, d); break;
		case kOpJalr:   legal = decodeJalr(inst, d);   break;
		case kOpSystem: legal = decodeSystem(inst, d); break;
		case kOpAmo:    legal = decodeAmo(inst, d);    break;

		case kOpLui:
			d.left = 0;
			d.right = getImmUType(inst);
			d.rd = getRd(inst);
			legal = 1;
			break;
		case kOpAuipc:
			d.left = static_cast<std::int32_t>(m_fetchLatch.currPC);
			d.right = getImmUType(inst);
			d.rd = getRd(inst);
			legal = 1;
			break;
		case kOpJal:
			d.disp_strval = getImmJType(inst);
			d.rd = getRd(inst);
			legal = 1;
			break;
		default:
			break;
		}
	}

	if (!legal) {
		d = Instruction{};
		d.inst = inst;
		out.illegalInst = 1;
	}
	return out;
}

std::int32_t Decode::readReg(std::uint8_t idx) const
{
	return m_registerFile.read(idx);
}

//Decode functions return 0 when illegal instruction is encountered
//and returns 1 when the function is legal after parsing
//decodeInst calls the decode function and handles returned legality
std::uint8_t Decode::decodeReg(std::uint32_t inst, Instruction& d) const
{
	const std::uint8_t funct3 = getFunct3(inst);
	const std::uint8_t funct7  = getFunct7(inst);
	AluOp aluOp;

	if (funct7 == kMulDivFunct7)
	{
		if (!m_extM) { return 0; }
		switch(funct3) {
		case kMulFunct3:
		case kMulhFunct3:
		case kMulhsuFunct3:
		case kMulhuFunct3:
			aluOp = AluOp::Mul;
			break;
		case kDivFunct3:
			aluOp = AluOp::Div;
			break;
		case kDivuFunct3:
			aluOp = AluOp::DivU;
			break;
		case kRemFunct3:
			aluOp = AluOp::Rem;
			break;
		case kRemuFunct3:
			aluOp = AluOp::RemU;
			break;
		default:
			return 0; //should never be reached, but illegal inst if so (could also be a fatalError)
		}
	}

	else if (funct7 == kAluOpFunct7)
	{
		switch (funct3) {
		case kAdd_SubFunct3:
			aluOp = AluOp::Add;
			break;
		case kSllFunct3:
			aluOp = AluOp::LeftShift;
			break;
		case kSltFunct3:
		case kSltuFunct3:
			aluOp = AluOp::Sub;
			break;
		case kSrFunct3:
			aluOp = AluOp::RightShiftL;
			break;
		case kOrFunct3:
			aluOp = AluOp::Or;
			break;
		case kXorFunct3:
			aluOp = AluOp::Xor;
			break;
		case kAndFunct3:
			aluOp = AluOp::And;
			break;
		default:
			return 0;
		}
	}
	else if (funct7 == kSra_SubFunct7)
	{
		if (funct3 == kAdd_SubFunct3) { aluOp = AluOp::Sub; }
		else if (funct3 == kSrFunct3) { aluOp = AluOp::RightShiftA; }
		else { return 0; }
	}
	else
	{
		return 0; //illegal funct7
	}

	//legal, set Instruction fields
	d.left = readReg(getRs1(inst));
	d.right = readReg(getRs2(inst));
	d.rd = getRd(inst);
	d.aluop = static_cast<std::uint8_t>(aluOp);
	return 1;
}

std::uint8_t Decode::decodeImm(std::uint32_t inst, Instruction& d) const
{
	const std::uint8_t funct3 = getFunct3(inst);
	const std::uint8_t funct7 = getFunct7(inst);
	AluOp aluOp;

	//Shifts are the only funct3 that have a funct7 (imm[11:5])
	switch (funct3) {
	case kAdd_SubFunct3:
		aluOp = AluOp::Add;
		d.right = getImmIType(inst);
		break;
	case kSllFunct3:
		if (funct7 != kAluOpFunct7) { return 0; }
		aluOp = AluOp::LeftShift;
		d.right = static_cast<std::int32_t>(getRs2(inst));
		break;
	case kSltFunct3:
	case kSltuFunct3:
		aluOp = AluOp::Sub;
		d.right = getImmIType(inst); //signedness handled later in different stage
		break;
	case kSrFunct3:
		if(funct7 == kSra_SubFunct7) { aluOp = AluOp::RightShiftA; }
		else if (funct7 == kAluOpFunct7){ aluOp = AluOp::RightShiftL; }
		else { return 0; } //illegal funct7
		d.right = static_cast<std::int32_t>(getRs2(inst));
		break;
	case kOrFunct3:
		aluOp = AluOp::Or;
		d.right = getImmIType(inst);
		break;
	case kXorFunct3:
		aluOp = AluOp::Xor;
		d.right = getImmIType(inst);
		break;
	case kAndFunct3:
		aluOp = AluOp::And;
		d.right = getImmIType(inst);
		break;
	default:
		return 0;
	}

	d.left = readReg(getRs1(inst));
	d.rd = getRd(inst);
	d.aluop = static_cast<std::uint8_t>(aluOp);
	return 1;
}

std::uint8_t Decode::decodeLoad(std::uint32_t inst, Instruction& d) const
{
	const std::uint8_t funct3 = getFunct3(inst);
	//     lb | lh | lw		||		 lbu		||		 lhu			== legal
	if (!(funct3 <= 0b10u	|| funct3 == 0b100u || funct3 == 0b101u)) { return 0; } //illegal
	
	d.left = readReg(getRs1(inst));
	d.right = getImmIType(inst);
	d.rd = getRd(inst);
	d.aluop = static_cast<std::uint8_t>(AluOp::Add);
	d.memop = static_cast<std::uint8_t>((funct3 & 0b11u) + 1u); //funct3 to kMemop
	return 1;
}

std::uint8_t Decode::decodeStore(std::uint32_t inst, Instruction& d) const
{
	const std::uint32_t funct3 = getFunct3(inst);
	//	sb | sh | sw	 == legal
	if (funct3 > 0b10u) { return 0; } //illegal

	d.left = readReg(getRs1(inst));
	d.right = getImmSType(inst);
	d.disp_strval = readReg(getRs2(inst));
	d.rd = 0;
	d.aluop = static_cast<std::uint8_t>(AluOp::Add);
	d.memop = static_cast<std::uint8_t>(0b100u | ((funct3 & 0b11u) + 1u)); //funct3 to kMemop
	return 1;
}

std::uint8_t Decode::decodeBranch(std::uint32_t inst, Instruction& d) const
{
	const std::uint32_t funct3 = getFunct3(inst);
	if (funct3 == 0b10u || funct3 == 0b11u) { return 0; } //branch instruction doesnt exist

	d.left = readReg(getRs1(inst));
	d.right = readReg(getRs2(inst));
	d.disp_strval = getImmBType(inst);
	d.rd = 0;
	d.aluop = static_cast<std::uint8_t>(AluOp::Sub);
	return 1;
}

std::uint8_t Decode::decodeJalr(std::uint32_t inst, Instruction& d) const
{
	if (getFunct3(inst) != 0b0u) { return 0; } //illegal

	d.left = readReg(getRs1(inst));
	d.right = getImmIType(inst);
	d.rd = static_cast<std::uint8_t>(getRd(inst));
	d.aluop = static_cast<std::uint8_t>(AluOp::Add);
	return 1;
}

std::uint8_t Decode::decodeSystem(std::uint32_t inst, Instruction& d) const
{
	const std::uint32_t funct3 = getFunct3(inst);
	const std::uint8_t rd = getRd(inst); //called early because we need it to check fence and for inst field
	if (funct3 == 0b0u) {
		//ECALL is 0 in every position except opcode
		if (inst == static_cast<std::uint32_t>(kOpSystem)) { return 1; }
		if (getFunct7(inst) == kSFenceFunct7 && rd == kSFenceRd)	//funct3, funct7 and rd == FENCE.VMA
		{ 
			d.left = readReg(getRs1(inst));							//virtual address
			d.right = readReg(getRs2(inst));						//asid
			return 1;
		}
		return 0;
	}

	if (funct3 == 0b100u) { return 0; }

	// [csrrw (0b001) | csrrs (0b010) | csrrc (0b011) ] = rs1 || [csrrwi (0b101) | csrrsi (0b110) | csrrci (0b111) ] = uimm (in rs1 position)
	d.left = (funct3 & 0b100u) ? static_cast<std::int32_t>(getRs1(inst)) : readReg(getRs1(inst));
	d.right = getCSR(inst);
	d.rd = rd;
	return 1;
}

std::uint8_t Decode::decodeAmo(std::uint32_t inst, Instruction& d) const
{
	if (!m_extA) { return 0; }
	if (getFunct3(inst) != kAmoFunct3) { return 0; }

	const std::uint32_t funct5 = getFunct5(inst);
	const std::uint8_t rs2 = getRs2(inst);

	switch (funct5) {
	case kAmoSwapFunct5:
	case kAmoAddFunct5:
	case kAmoXorFunct5:
	case kAmoAndFunct5:
	case kAmoOrFunct5:
	case kAmoMinuFunct5:
	case kAmoMaxuFunct5:
	case kSCFunct5:
		break;
	case kLRFunct5:
		if (rs2 != 0b00000) { return 0; }
		break;
	default: 
		return 0;
	}

	d.left = readReg(getRs1(inst));
	d.right = readReg(rs2);
	d.rd = getRd(inst);
	d.aluop = static_cast<std::uint8_t>(AluOp::Add);
	d.memop = kMemopAmo;
	return 1;
}
