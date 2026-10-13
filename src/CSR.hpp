#pragma once
#include <cstdint>
#include <chrono>

class CSR
{
public:
    std::uint32_t read(std::uint16_t CSRNumber) const;
    std::uint8_t write(std::uint16_t CSRNumber, std::uint32_t CSRValue);
    void incCycle();
    void incInstret();
    CSR(std::uint32_t mhartid, std::uint8_t mmuState);

private:
    static constexpr uint16_t k_mhartid = 0xF14;
    static constexpr uint16_t k_satp = 0x180;
    static constexpr uint16_t k_time = 0xC01;
    static constexpr uint16_t k_timeh = 0xC81;
    static constexpr uint16_t k_cycle = 0xC00;
    static constexpr uint16_t k_cycleh = 0xC80;
    static constexpr uint16_t k_instret = 0xC02;
    static constexpr uint16_t k_instreth = 0xC82;

    std::uint32_t readmhartid() const;
    std::uint32_t readsatp() const;
    std::uint32_t readtime() const;
    std::uint32_t readtimeh() const;
    std::uint32_t readcycle() const;
    std::uint32_t readcycleh() const;
    std::uint32_t readinstret() const;
    std::uint32_t readinstreth() const;

    void writesatp(std::uint32_t CSRValue);

    std::uint32_t m_mhartid;
    std::uint8_t  m_mmuState; 
    std::uint32_t m_satp;
    std::uint64_t m_cycle;
    std::uint64_t m_instret;

    //dont need a m_time variable, because we'd just be setting it to UNIX time, then referencing the variable
    //instead, just reference UNIX time directly 
};


