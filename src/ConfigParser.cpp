#include "ConfigParser.hpp"
#include "HelperFunctions.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string_view>
#include <unordered_set>
#include <cinttypes>

//anon namespace wraps functions used for returning conversions
namespace {

    //if a flag appears in a config file that is not found in this set, return a fatal error
    const std::unordered_set<std::string> flagsConfigFile =
    {
            "m", "a", "mem", "harts", "hz",
            "cL", "c1", "c2", "c3",
            "cS1", "cS2", "cS3",
            "cB1", "cB2", "cB3",
            "cW1", "cW2", "cW3",
            "cE1", "cE2", "cE3",
            "cC", "bp", "bpD",
            "mmu", "tlb", "tlbE"
    };

    //flags only found in CLI, when checking CLI flags, we will use both flagsCLI and flagsConfigFile
    const std::unordered_set<std::string> flagsCLI = { "f", "i", "o" };

    //default flag values
    std::map<std::string, std::string> flagValueMap
    {
        {"f", "" },
        {"i", ""}, //REQUIRED
        {"o", "-"}, //- uses stdout
        {"m", "on"},
        {"a", "on"},
        {"mem", "1M"},
        {"harts", "4"},
        {"hz", "100"},
        {"cL", "3"},
        {"c1", "dm"},
        {"c2", "sa,3"},
        {"c3", "fa"},
        {"cS1", "1K"},
        {"cS2", "2K"},
        {"cS3", "4K"},
        {"cB1", "4"},
        {"cB2", "5"},
        {"cB3", "6"},
        {"cW1", "wt"},
        {"cW2", "wt"},
        {"cW3", "wb"},
        {"cE1", "fifo"},
        {"cE2", "lru"},
        {"cE3", "lfu"},
        {"cC", "snoop"},
        {"bp", "64"},
        {"bpD", "0"},
        {"mmu", "on"},
        {"tlb", "8"},
        {"tlbE", "lru"}
    };

    //When CLI sets a flag, we set the flag to dirty
    //When a flag is read from a config file, it checks if the flag is dirty before setting
    //flags set from the config file do not set the dirty bit, because the latest flag takes precedence
    std::map<std::string, uint8_t> flagDirtyBit
    {
        {"f", 0},
        {"i", 0},
        {"o", 0},
        {"m", 0},
        {"a", 0},
        {"mem", 0},
        {"harts", 0},
        {"hz", 0},
        {"cL", 0},
        {"c1", 0},
        {"c2", 0},
        {"c3", 0},
        {"cS1", 0},
        {"cS2", 0},
        {"cS3", 0},
        {"cB1", 0},
        {"cB2", 0},
        {"cB3", 0},
        {"cW1", 0},
        {"cW2", 0},
        {"cW3", 0},
        {"cE1", 0},
        {"cE2", 0},
        {"cE3", 0},
        {"cC", 0},
        {"bp", 0},
        {"bpD", 0},
        {"mmu", 0},
        {"tlb", 0},
        {"tlbE", 0}
    };

    //Flag Convertion functions called in flagValueToConfigParser
    //Take in string of flagValue and output a hard value or a enum to set a variable from the ConfigParser to
    //used in -m, -a, -mmu flags
    ConfigParser::OnOff convOnOff(const std::string& flagValue) {
        static const std::map <std::string, ConfigParser::OnOff>  onOffMap
        {
            {"on",  ConfigParser::OnOff::On},
            {"off", ConfigParser::OnOff::Off}
        };

        auto it = onOffMap.find(flagValue);
        if (it != onOffMap.end()) {
            return it->second;
        }

        //flag value invalid
        helper::fatalError("Invalid flag value should be \"on\" or \"off\" (" + flagValue + ")");
    }

    //used in convBytes and convHertz
    //suffixMult is the amount to multiply for each increment of a suffix (1000 or 1024 for hertz or bytes)
    //https://en.cppreference.com/cpp/utility/from_chars used to reference from_chars and std::string_quoted
    int32_t convSuffixValue(const std::string& flagValue, const std::string& flagUnit, const uint32_t suffixMult) {
        //stoull is better (and more readable), but is not architecture agnostic :(
        int32_t uint32tFlagValue;
        std::from_chars_result fcr; //has members ptr and ec

        fcr = std::from_chars(flagValue.data(), flagValue.data() + helper::int32tStringSize(flagValue), uint32tFlagValue);
        if (fcr.ec == std::errc::invalid_argument) {
            helper::fatalError("Invalid flag value should be some unsigned number of " + flagUnit + " followed by suffix none/K/M/G (" + flagValue + ")");
        }
        else if (fcr.ec == std::errc::result_out_of_range) {
            helper::fatalError("Invalid flag value is too large (" + flagValue + ")");
        }
        else if (fcr.ec != std::errc()) {
            helper::fatalError("Error on " + flagValue);
        }

        //make a string view of the substring AFTER the number (suffix/empty on success, other results are errors)
        std::string_view suffix(fcr.ptr);

        //flag has no suffix
        if (helper::int32tStringSize(suffix) == 0) {
            return uint32tFlagValue;
        }

        //flag has a suffix
        else if (helper::int32tStringSize(suffix) == 1) {
            switch (suffix[0]) {
            case 'K':
                return uint32tFlagValue * suffixMult;
            case 'M':
                return uint32tFlagValue * suffixMult * suffixMult;
            case 'G':
                return uint32tFlagValue * suffixMult * suffixMult * suffixMult;
                //suffix is invalid
            default:
                helper::fatalError("Invalid flag value should be some unsigned number of " + flagUnit + " followed by suffix none/K/M/G (" + flagValue + ")");
            }
        }

        //flag has non-integer, non-suffix values (ie. has some character(s) within the value that is NOT at the end of the value)
        else {
            helper::fatalError("Invalid flag value should be some unsigned number of " + flagUnit + " followed by suffix none/K/M/G (" + flagValue + ")");
        }
    }

    //Takes no suffix, K, M, or G and returns the size of the flagValue in bytes
    //used in -mem, -cS1, -cS2, -cS3 flags
    uint32_t convBytes(const std::string& flagValue) {
        uint32_t bytes = convSuffixValue(flagValue, "bytes", 1024);
        return bytes;
    }

    //convert to an int, then check if int falls in range given
    //if no range is given, the value is not ranged
    //used in -harts, -cL, -cbP             (ranged)
    //used in -cB1, -cB2, -cB3, -bp, -tlb   ("unranged") 
    uint32_t convRangedUint32(const std::string& flagValue, uint32_t lower = 0, uint32_t upper = ~static_cast<uint32_t>(0)) {
        uint32_t convValue;
        std::from_chars_result fcr; //has members ptr and ec

        fcr = std::from_chars(flagValue.data(), flagValue.data() + helper::int32tStringSize(flagValue), convValue);
        if (fcr.ec == std::errc::invalid_argument) {
            helper::fatalError("Invalid flag value should be some unsigned number (" + flagValue + ")");
        }
        else if (fcr.ec == std::errc::result_out_of_range) {
            helper::fatalError("Invalid flag value is too large (" + flagValue + ")");
        }
        else if (fcr.ec != std::errc()) {
            helper::fatalError("Error on " + flagValue);
        }

        std::string_view substr(fcr.ptr);

        //some non-integer value mixed in with flagValue
        if (helper::int32tStringSize(substr) != 0) {
            helper::fatalError("Invalid flag value should be some unsigned number (" + flagValue + ")");
        }
        if (convValue < lower) {
            helper::fatalError("Flag value lies below lower range (" + flagValue + " < " + std::to_string(lower) + ")");
        }
        if (convValue > upper) {
            helper::fatalError("Flag value lies above upper range (" + flagValue + " > " + std::to_string(upper) + ")");
        }

        return convValue;
    }

    //used in -hz
    uint32_t convHertz(const std::string& flagValue) {
        uint32_t hertz = convSuffixValue(flagValue, "hertz", 1000);
        return hertz;
    }

    //maps cache type dm, sa, fa to CacheType enum, and sets ways-exponent in the case of sa flag
    //used in -c1, -c2, -c3
    ConfigParser::CacheTypeSetting convCacheType(const std::string& flagValue) {
        static const std::map <std::string, ConfigParser::CacheType>  cacheTypeMap
        {
            {"dm",  ConfigParser::CacheType::DirectMapped},
            {"sa,", ConfigParser::CacheType::SetAssociative},
            {"fa",  ConfigParser::CacheType::FullyAssociative}
        };

        //dm and fa are 2 characters long, so substr will catch them (and any other text that may invalidate them)
        //sa, is 3 characters long, and substr will catch this substr without reaching ways-exponent
        auto it = cacheTypeMap.find(flagValue.substr(0, 3));
        if (it != cacheTypeMap.end()) {

            int32_t waysExponent = 0;

            if (it->second == ConfigParser::CacheType::SetAssociative) {
                std::string waysSubStr = flagValue.substr(3);
                std::from_chars_result fcr = std::from_chars(waysSubStr.data(), waysSubStr.data() + helper::int32tStringSize(waysSubStr), waysExponent);

                if (fcr.ec == std::errc::invalid_argument) {
                    helper::fatalError("Invalid flag value should be \"sa,\" followed by some unsigned number (" + flagValue + ")");
                }
                else if (fcr.ec == std::errc::result_out_of_range) {
                    helper::fatalError("Invalid flag value is too large (" + flagValue + ")");
                }
                else if (fcr.ec != std::errc()) {
                    helper::fatalError("Error on " + flagValue);
                }

                std::string_view substr(fcr.ptr);

                //some non-integer value mixed in with flagValue
                if (helper::int32tStringSize(substr) != 0) {
                    helper::fatalError("Invalid flag value should be \"sa,\" followed by some unsigned number (" + flagValue + ")");
                }
            }
            return { it->second , waysExponent };
        }

        //flag value invalid
        helper::fatalError("Invalid flag value should be \"dm\", \"sa,#\" or \"fa\" (" + flagValue + ")");
    }

    //Maps wb and wt to WriteBack and WriteThrough enums
    //used in -cW1, -cW2, -cW3
    ConfigParser::WritePolicy convWritePolicy(const std::string& flagValue) {
        static const std::map <std::string, ConfigParser::WritePolicy>  writePolicyMap
        {
            {"wb", ConfigParser::WritePolicy::WriteBack},
            {"wt", ConfigParser::WritePolicy::WriteThrough}
        };

        auto it = writePolicyMap.find(flagValue);
        if (it != writePolicyMap.end()) {
            return it->second;
        }

        //flag value invalid
        helper::fatalError("Invalid flag value should be \"wb\" or \"wt\" (" + flagValue + ")");
    }

    //Maps fifo, lru, and lfu to EvictionPolicy enums
    //used in -cE1, -cE2, -cE3, -tlbE
    ConfigParser::EvictionPolicy convEvictionPolicy(const std::string& flagValue) {
        static const std::map <std::string, ConfigParser::EvictionPolicy>  evictionPolicyMap
        {
            {"fifo", ConfigParser::EvictionPolicy::FIFO},
            {"lru",  ConfigParser::EvictionPolicy::LRU},
            {"lfu",  ConfigParser::EvictionPolicy::LFU}
        };

        auto it = evictionPolicyMap.find(flagValue);
        if (it != evictionPolicyMap.end()) {
            return it->second;
        }

        //flag value invalid
        helper::fatalError("Invalid flag value should be \"fifo\", \"lru\" or \"lfu\" (" + flagValue + ")");
    }

    //Maps none, snoop, and dir to CoherencyProtocol enums
    //used in -cC
    ConfigParser::CoherencyProtocol convCoherency(const std::string& flagValue) {
        static const std::map <std::string, ConfigParser::CoherencyProtocol>  coherencyProtocolMap
        {
            {"none",    ConfigParser::CoherencyProtocol::None},
            {"snoop",   ConfigParser::CoherencyProtocol::Snoop},
            {"dir",     ConfigParser::CoherencyProtocol::Dir}
        };

        auto it = coherencyProtocolMap.find(flagValue);
        if (it != coherencyProtocolMap.end()) {
            return it->second;
        }

        //flag value invalid
        helper::fatalError("Invalid flag value should be \"none\", \"snoop\" or \"dir\" (" + flagValue + ")");
    }

    ConfigParser::BranchDefault convBranchDefault(const std::string& flagValue) {
        uint8_t bpDVal = static_cast<uint8_t>(convRangedUint32(flagValue, 0, 3));
        switch (bpDVal) {
        case 0:
            return ConfigParser::BranchDefault::StronglyNotTaken;
        case 1:
            return ConfigParser::BranchDefault::WeaklyNotTaken;
        case 2:
            return ConfigParser::BranchDefault::WeaklyTaken;
        case 3:
            return ConfigParser::BranchDefault::StronglyTaken;
            //default not needed, values outside of 0-3 will throw in convRangedUint32
        }
    }

    //does not return anything, helper::fatalError() on invalid, return from func on valid
    void checkValid(const std::string& flag, const std::string& flagValue) {
        if (flag == "f" || flag == "i") {
            std::ifstream fin(flagValue);
            if (!fin.is_open()) {
                helper::fatalError("Could not open file (" + flagValue + ")");
            }
            fin.close();
        }
        else if (flag == "o") {
            if (flagValue != "-") {
                std::ofstream fout(flagValue, std::ios::app);
                if (!fout.is_open()) {
                    helper::fatalError("Could not open file (" + flagValue + ")");
                }
                fout.close();
            }
        }
        else if (flag == "m" || flag == "a" || flag == "mmu") {
            convOnOff(flagValue);
        }
        else if (flag == "mem" || flag == "cS1" || flag == "cS2" || flag == "cS3") {
            convBytes(flagValue);
        }
        else if (flag == "harts") {
            convRangedUint32(flagValue, 1, 16);
        }
        else if (flag == "hz") {
            convHertz(flagValue);
        }
        else if (flag == "cL") {
            convRangedUint32(flagValue, 0, 3);
        }
        else if (flag == "c1" || flag == "c2" || flag == "c3") {
            convCacheType(flagValue);
        }
        else if (flag == "cW1" || flag == "cW2" || flag == "cW3") {
            convWritePolicy(flagValue);
        }
        else if (flag == "cE1" || flag == "cE2" || flag == "cE3" || flag == "tlbE") {
            convEvictionPolicy(flagValue);
        }
        else if (flag == "cC") {
            convCoherency(flagValue);
        }
        else if (flag == "cB1" || flag == "cB2" || flag == "cB3" || flag == "bp" || flag == "tlb") {
            convRangedUint32(flagValue);
        }
        else if (flag == "bpD") {
            convBranchDefault(flagValue);
        }
        else {
            helper::fatalError("Unkown flag passed to checkValid (" + flagValue + ")");
        }
    }

    //validate that flag has a "-", 
    //then validate that flag exists in CLI or configFile set
    //then validate that flag has a value
    //we do not check if values are valid until before they are set in the ConfigParser struct
    void parseCLI(int32_t argc, char** argv)
    {
        int32_t argi = 1;
        while (argi < argc) {
            std::string flag = argv[argi];
            if (flag[0] != '-') {
                helper::fatalError("Bad flag: (" + flag + ")");
            }

            flag = flag.substr(1);
            if (!(flagsConfigFile.contains(flag) || flagsCLI.contains(flag))) {
                helper::fatalError("Bad flag: (-" + flag + ")");
            }

            if (argi + 1 >= argc) {
                helper::fatalError("Flag has no value: (-" + flag + ")");
            }

            //check if the flag + value are valid NOW, throw an error even if it will later be overwritten
            checkValid(flag, argv[argi + 1]);

            flagValueMap[flag] = argv[argi + 1];
            flagDirtyBit[flag] = 1;
            argi += 2;
        }
    }

    //read from config file
    void parseConfFile()
    {
        std::ifstream fin(flagValueMap.at("f"));
        if (!fin.is_open()) {
            helper::fatalError("Could not open config file (" + flagValueMap.at("f") + ")");
        }

        std::string line;
        std::stringstream ss;

        while (std::getline(fin, line))
        {
            ss << line;
            std::string flag;
            std::string flagValue;
            std::string commentString;

            //check FIRST STRING (Flag)
            if (ss >> flag)
            {
                //ss was given a line with a comment
                if (flag[0] == '#')
                {
                    ss.str("");
                    ss.clear();
                    continue;
                }

                //ss was given a flag that does not exist (or is not allowed)
                if (!(flagsConfigFile.contains(flag)))
                {
                    helper::fatalError("Bad flag in config file: (" + flag + ")");
                }
            }
            else
            {
                //ss was given a line of all whitespace
                ss.str("");
                ss.clear();
                continue;
            }

            //check SECOND STRING (Flag Value)
            uint8_t hasComment = false;
            if (ss >> flagValue)
            {
                //if there is an inline with NO whitespace in flagValue, take a substr up to that position as true value
                int32_t commentPos = helper::int32tStringFind(flagValue, '#');
                hasComment = (commentPos != helper::int32_t_npos); //find == npos if # is not found
                flagValue = flagValue.substr(0, commentPos); //this does not change if there is no #

                //flag value was a comment, and does not exist
                if (helper::int32tStringSize(flagValue) == 0) {
                    helper::fatalError("Bad flag value in config file: (" + flag + ")");
                }

                //we check if the flag is valid even if it will not write to final config
                checkValid(flag, flagValue);
                if (flagDirtyBit[flag] == 0) {
                    //flag value with stripped comment (if there was one)
                    flagValueMap[flag] = flagValue;
                }
            }
            //flag has no value, throw fatal error
            else
            {
                helper::fatalError("Bad flag value in config file: (" + flag + ")");
            }

            //check THIRD STRING (if applicable, MUST be a comment)
            if (ss >> commentString) {
                if (commentString[0] != '#' && !hasComment) {
                    helper::fatalError("Non-comment string after flag value in config file: (" + flag + ")");
                }
            }

            ss.str("");
            ss.clear();
        }
    }

    //convert flag values from map to their respective enums in ConfigParser
    ConfigParser flagValueToConfigParser() {
        ConfigParser cfg;

        cfg.m_configFilePath = flagValueMap.at("f");
        cfg.m_inputElfPath = flagValueMap.at("i");
        cfg.m_outputPath = flagValueMap.at("o");

        cfg.m_extM = convOnOff(flagValueMap.at("m"));
        cfg.m_extA = convOnOff(flagValueMap.at("a"));

        cfg.m_memBytes = convBytes(flagValueMap.at("mem"));

        cfg.m_numHarts = convRangedUint32(flagValueMap.at("harts"), 1, 16);
        cfg.m_hz = convHertz(flagValueMap.at("hz"));

        cfg.m_cacheLevels = convRangedUint32(flagValueMap.at("cL"), 0, 3);
        cfg.m_cache1 = convCacheType(flagValueMap.at("c1"));
        cfg.m_cache2 = convCacheType(flagValueMap.at("c2"));
        cfg.m_cache3 = convCacheType(flagValueMap.at("c3"));

        cfg.m_cacheSize1 = convBytes(flagValueMap.at("cS1"));
        cfg.m_cacheSize2 = convBytes(flagValueMap.at("cS2"));
        cfg.m_cacheSize3 = convBytes(flagValueMap.at("cS3"));

        // Block size exponents are just non-negative small integers (2^N bytes).
        cfg.m_blockSizeExp1 = convRangedUint32(flagValueMap.at("cB1"));
        cfg.m_blockSizeExp2 = convRangedUint32(flagValueMap.at("cB2"));
        cfg.m_blockSizeExp3 = convRangedUint32(flagValueMap.at("cB3"));

        cfg.m_writePolicy1 = convWritePolicy(flagValueMap.at("cW1"));
        cfg.m_writePolicy2 = convWritePolicy(flagValueMap.at("cW2"));
        cfg.m_writePolicy3 = convWritePolicy(flagValueMap.at("cW3"));

        cfg.m_evictionPolicy1 = convEvictionPolicy(flagValueMap.at("cE1"));
        cfg.m_evictionPolicy2 = convEvictionPolicy(flagValueMap.at("cE2"));
        cfg.m_evictionPolicy3 = convEvictionPolicy(flagValueMap.at("cE3"));

        cfg.m_coherency = convCoherency(flagValueMap.at("cC"));

        cfg.m_branchPredictorRows = convRangedUint32(flagValueMap.at("bp"));
        cfg.m_branchDefault = convBranchDefault(flagValueMap.at("bpD"));

        cfg.m_mmuState = convOnOff(flagValueMap.at("mmu"));
        cfg.m_tlbSlots = convRangedUint32(flagValueMap.at("tlb"));
        cfg.m_tlbEviction = convEvictionPolicy(flagValueMap.at("tlbE"));

        return cfg;
    }

} 

//flagValueMap stores maps flags to their values
//default value in map is already set
//In parseConfig, we will read from CLI, set flags (error if no -i) and dirtyBitMap
//then read from configFile if -f flag was included in CLI, check if flag is dirty, and set if not
//if flag is dirty, it was set by CLI, and takes precedence, flags set by configFile do not set dirty
//bit, so that latest repeated flags take precedence.
namespace cfg {
    ConfigParser parseConfig(int32_t argc, char** argv, std::uint8_t debug) {
        parseCLI(argc, argv);
        //if -i is not set, throw an error
        //if debug is on, allow passing with no ELF file, remove this in future parts when we have access to ELF files
        if (flagDirtyBit["i"] == 0 && !debug) {
            helper::fatalError("No Input ELF file provided with -i");
        }

        //if -f flag is set, we parse the given configFile
        if (flagDirtyBit["f"] == 1) {
            parseConfFile();
        }

        //all flag values are set : CLI > Config File > Defaults
        //now, we set flag values to ConfigParser variables
        //each time we set a value, we verify that the value is legal

        return flagValueToConfigParser();
    }

    const void printConfigStringTypes(ConfigParser conf) 
    {
        {
            for (const auto& [fk, fv] : flagValueMap) {
                printf("Flag -%s: %s\n", fk.c_str(), fv.c_str());
            }
        }

    }

    const void printConfigUnderlyingTypes(ConfigParser conf)
    {
        printf("Flag -f: %s\n"
            "Flag -i: %s\n"
            "Flag -o: %s\n"
            "Flag -m: %" PRIu8"\n"
            "Flag -a: %" PRIu8"\n"
            "Flag -mem: %" PRIu32"\n"
            "Flag -harts: %" PRIu32"\n"
            "Flag -hz: %" PRIu32"\n"
            "Flag -cL: %" PRIu32"\n"
            "Flag -c1: %" PRIu8",%" PRIu32 "\n"
            "Flag -c2: %" PRIu8",%" PRIu32 "\n"
            "Flag -c3: %" PRIu8",%" PRIu32 "\n"
            "Flag -cS1: %" PRIu32 "\n"
            "Flag -cS2: %" PRIu32 "\n"
            "Flag -cS3: %" PRIu32 "\n"
            "Flag -cB1: %" PRIu32 "\n"
            "Flag -cB2: %" PRIu32 "\n"
            "Flag -cB3: %" PRIu32 "\n"
            "Flag -cW1: %" PRIu8"\n"
            "Flag -cW2: %" PRIu8"\n"
            "Flag -cW3: %" PRIu8"\n"
            "Flag -cE1: %" PRIu8"\n"
            "Flag -cE2: %" PRIu8"\n"
            "Flag -cE3: %" PRIu8"\n"
            "Flag -cC: %" PRIu8"\n"
            "Flag -bp: %" PRIu32"\n"
            "Flag -bpD: %" PRIu8"\n"
            "Flag -mmu: %" PRIu8"\n"
            "Flag -tlb: %" PRIu32"\n"
            "Flag -tlbE: %" PRIu8"\n"
            , conf.m_configFilePath.c_str(), conf.m_inputElfPath.c_str(), conf.m_outputPath.c_str(), std::to_underlying(conf.m_extM), std::to_underlying(conf.m_extA)
            , conf.m_memBytes, conf.m_numHarts, conf.m_hz, conf.m_cacheLevels, std::to_underlying(conf.m_cache1.kind), conf.m_cache1.waysExponent
            , std::to_underlying(conf.m_cache2.kind), conf.m_cache2.waysExponent, std::to_underlying(conf.m_cache3.kind), conf.m_cache3.waysExponent
            , conf.m_cacheSize1, conf.m_cacheSize2, conf.m_cacheSize3, conf.m_blockSizeExp1, conf.m_blockSizeExp2, conf.m_blockSizeExp3
            , std::to_underlying(conf.m_writePolicy1), std::to_underlying(conf.m_writePolicy2), std::to_underlying(conf.m_writePolicy3)
            , std::to_underlying(conf.m_evictionPolicy1), std::to_underlying(conf.m_evictionPolicy2), std::to_underlying(conf.m_evictionPolicy3)
            , std::to_underlying(conf.m_coherency), conf.m_branchPredictorRows, std::to_underlying(conf.m_branchDefault), std::to_underlying(conf.m_mmuState)
            , conf.m_tlbSlots, std::to_underlying(conf.m_tlbEviction));
    }
}