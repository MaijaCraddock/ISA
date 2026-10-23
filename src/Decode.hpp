#pragma once
#include <cstdint>
#include "Fetch.hpp"
#include "RegisterFile.hpp"

enum class AluOp : std::uint8_t {                                 //funct7          funct3
    Add,         // Signed addition                                 0b0000000       0b000
    Sub,         // Signed subtraction (or comparison for BRANCH)   0b0100000       0b000
    Mul,         // Signed multiplication                           0b0000001       0b000 | 0b001 | 0b010 | 0b011
    Div,         // Signed division                                 0b0000001       0b100
    DivU,        // Unsigned division                               0b0000001       0b101
    Rem,         // Signed remainder                                0b0000001       0b110
    RemU,        // Unsigned remainder (modulo)                     0b0000001       0b111
    LeftShift,   // Logical left shift                              0b0000000       0b001
    RightShiftA, // Arithmetic right shift                          0b0100000       0b101
    RightShiftL, // Logical right shift                             0b0000000       0b101
    Or,          // Bitwise inclusive OR                            0b0000000       0b110
    Xor,         // Bitwise exclusive OR                            0b0000000       0b100
    And,         // Bitwise AND                                     0b0000000       0b111
};

enum class InstType : std::uint8_t { R, I, U, S, B, J, Illegal };

struct DecodeIntermediateRegister
{
    FetchIntermediateRegister latch;
    std::uint8_t illegalInst = 0;
};

class Decode
{
public:
    //memops need to visible for Memory stage, so they dont exist in anon namespace like the other constexpr
    //not as an enum because std::uint8_t memop in Instruction would require a cast each time we want to use it
    //Table 5. Instruction Structure defines these values (aside from AMO, which could have been a default case otherwise)
    static constexpr std::uint8_t kMemopNone = 0b0000;
    static constexpr std::uint8_t kMemopLoadByte = 0b0001;  //funct3 = 0b000 
    static constexpr std::uint8_t kMemopLoadHalf = 0b0010;  //funct3 = 0b001 
    static constexpr std::uint8_t kMemopLoadWord = 0b0011;  //funct3 = 0b010 
    static constexpr std::uint8_t kMemopStoreByte = 0b0101; //funct3 = 0b000 
    static constexpr std::uint8_t kMemopStoreHalf = 0b0110; //funct3 = 0b001 
    static constexpr std::uint8_t kMemopStoreWord = 0b0111; //funct3 = 0b010 
    static constexpr std::uint8_t kMemopAmo = 0b1011;

    Decode(const RegisterFile& registerFile, const FetchIntermediateRegister& fetchLatch, std::uint8_t extM, std::uint8_t extA);

    void rising_edge();
    void falling_edge();

    const DecodeIntermediateRegister& outputLatch() const;

    void requestBubble();
    static DecodeIntermediateRegister bubble();

    static InstType getType(std::uint32_t inst);

private:
    const RegisterFile& m_registerFile;
    const FetchIntermediateRegister& m_fetchLatch;
    std::uint8_t m_extM;
    std::uint8_t m_extA;

    DecodeIntermediateRegister m_current;   //front buffer
    DecodeIntermediateRegister m_next;      //back buffer
    std::uint8_t m_bubbleRequested = 0;

    //calls specific decode on FetchIntermediateRegister (front latch of fetch)
    //if the instruction is illegal, we park, otherwise we prepare m_next
    //handled on rising edges
    DecodeIntermediateRegister decodeInst() const;
    std::int32_t readReg(std::uint8_t idx) const;

    //return 1 when legal and 0 when illegal
    std::uint8_t decodeReg(std::uint32_t inst, Instruction& d) const;
    std::uint8_t decodeImm(std::uint32_t inst, Instruction& d) const;
    std::uint8_t decodeLoad(std::uint32_t inst, Instruction& d) const;
    std::uint8_t decodeStore(std::uint32_t inst, Instruction& d) const;
    std::uint8_t decodeBranch(std::uint32_t inst, Instruction& d) const;
    std::uint8_t decodeJalr(std::uint32_t inst, Instruction& d) const;
    std::uint8_t decodeSystem(std::uint32_t inst, Instruction& d) const;
    std::uint8_t decodeAmo(std::uint32_t inst, Instruction& d) const;
};