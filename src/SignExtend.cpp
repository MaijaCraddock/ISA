#include "SignExtend.hpp"

std::int32_t SignExtend::signExtend(std::int32_t signedValue, std::uint8_t signBit)
{
    //shift the signedValue until the signBit is at bit 31 (0 indexed)
    std::uint8_t shiftCount = 31 - signBit;
    signedValue <<= shiftCount;

    //shift the same amount back to the right
    //if the sign bit was a 1, the right shift will insert 1's
    //otherwise, the right shift will insert 0's
    return signedValue >> shiftCount;
}
