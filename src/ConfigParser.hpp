#pragma once

#include <cstdint>
#include <string>
#include <utility> //anything using ConfigParser will want to translate Enum to underlying

//Probably a better solution than this, but when I used a class,
//I ended up using a lot of static functions, and it was cleaner as a struct

//Default variables are overwritten by the configFile if provided, and
//then overwritten for a possibly second time by CLI flags if provided.

//For the sake of clarity, default values will be noted in comments beside the variables
struct ConfigParser {
    //used in -m, -a, -mmu flags
    //note: bool is apparently NOT architecture agnostic
    enum class OnOff : std::uint8_t
    {
        Off = 0,
        On = 1
    };

    //used in -c1, -c2, -c3 flags
    enum class CacheType : std::uint8_t
    {
        DirectMapped,
        SetAssociative,
        FullyAssociative
    };

    //required for SetAssociate cache type, all other cache types still use this but ignore ways exponent 
    struct CacheTypeSetting {
        CacheType kind;
        int32_t waysExponent = 0;
    };

    //used in -cW1, -cW2, -cW3 flags
    enum class WritePolicy : std::uint8_t
    {
        WriteBack,
        WriteThrough
    };

    //used in -cE1, -cE2, -cE3, -tlbE flags
    enum class EvictionPolicy : std::uint8_t
    {
        FIFO,
        LRU,
        LFU
    };

    //used in -cC flag
    enum class CoherencyProtocol : std::uint8_t
    {
        None,
        Snoop,
        Dir
    };

    //used in -bpD flag
    enum class BranchDefault : std::uint8_t
    {
        StronglyNotTaken = 0,
        WeaklyNotTaken = 1,
        WeaklyTaken = 2,
        StronglyTaken = 3
    };

    //files are set only in CLI
    std::string m_configFilePath;         // -f def is none
    std::string m_inputElfPath;           // -i required, def is error
    std::string m_outputPath;             // -o def is stdout

    //RV32 extension toggles, throw "Illegal instruction" if a RV32M/A instruction is used while ext is set to off
    OnOff m_extM;                         // -m def is on
    OnOff m_extA;                         // -a def is on

    //RAM, HART and Hertz - define the machine settings
    uint32_t m_memBytes;                  // -mem   def is 1M = 1,048,576B
    uint32_t m_numHarts;                  // -harts def is 4, range [1,16]
    uint32_t m_hz;                        // -hz    def is 100 Hz

    //Cache levels (0 = no cache, 1 = L1 only, 2 = L1 and L2, 3 = L1, L2, and L3).
    uint32_t m_cacheLevels;                // -cL def is 3 

    //L1, L2, and L3 cache types
    CacheTypeSetting m_cache1;            // -c1 def is dm
    CacheTypeSetting m_cache2;            // -c2 def is sa,3
    CacheTypeSetting m_cache3;            // -c3 def is fa

    //L1, L2, and L3 cache sizes
    uint32_t m_cacheSize1;                // -cS1 def is 1K
    uint32_t m_cacheSize2;                // -cS2 def is 2K
    uint32_t m_cacheSize3;                // -cS3 def is 4K

    //L1, L2, and L3 cache block sizes (2^Exp)
    uint32_t m_blockSizeExp1 = 4;          // -cB1 def is 2^4  = 16B
    uint32_t m_blockSizeExp2 = 5;          // -cB2 def is 2^5  = 32B
    uint32_t m_blockSizeExp3 = 6;          // -cB3 def is 2^6  = 64B

    //L1, L2, and L3 cache write policies
    WritePolicy m_writePolicy1;           // -cW1 def is wt
    WritePolicy m_writePolicy2;           // -cW2 def is wt
    WritePolicy m_writePolicy3;           // -cW3 def is wb

    //L1, L2, and L3 eviction policies
    EvictionPolicy m_evictionPolicy1;     // -cE1 def is fifo
    EvictionPolicy m_evictionPolicy2;     // -cE2 def is lru
    EvictionPolicy m_evictionPolicy3;     // -cE3 def is lfu

    //Cache coherency protocol, MESI protocol will be used
    CoherencyProtocol m_coherency;        // -cC def is snoop

    //Number of rows in branch prediction table
    uint32_t m_branchPredictorRows;        // -bp def is 64, 0 disables prediction

    //Branch prediction default if branch addr is not in the table
    BranchDefault m_branchDefault;        // -bpD def is 0 (strongly not taken)

    //MMU state - on = can be turned on via satp register, off = satp register hardwired to 0
    OnOff m_mmuState;                     // -mmu def is on

    //Number of TLB slots and TLB eviction policy
    uint32_t m_tlbSlots;                   // -tlb def is 8(0 disables TLB)
    EvictionPolicy m_tlbEviction;         // -tlbE def is lru
};

//functions involved in returning/constructing/setting the ConfigParser are 
//scoped in an anonymous namespace in the .cpp file
namespace cfg {
    ConfigParser parseConfig(int32_t argc, char** argv, std::uint8_t debug=0);
    const void printConfigStringTypes(ConfigParser conf);
    const void printConfigUnderlyingTypes(ConfigParser conf);
}