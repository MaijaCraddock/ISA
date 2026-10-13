| Category                | Max | Earned |
|-------------------------|-----|--------|
| A — Build and Structure |    0.5 |  0.5      |
| B — ELF Header Parsing and Validation | 2.5    |  2.5      |
| C — Program Header Table Parsing |  2.0   |  2.0      |
| D — Segment Loading and Zero-Fill| 3.0 | 3.0|
| E — Entry Point| 1.0 | 1.0 |
| F — Robustness and Endianness | 1.0 | 1.0 |

## Justification ##

# A — Build and Structure #
All .cpp and .hpp files are compartmentalized and build properly with no warnings and no errors
Main code runs from Machine.cpp - only files that changed outside of changing uint64_t -> uint32_t were MachineCore.cpp/.hpp, Machine.cpp and the new ELFParser.cpp/.hpp files

# B — ELF Header Parsing and Validation #
ELFParser.cpp lines 6-12 define targets for e_ident, e_type, e_machine, and e_bitsize in endian-ness of host machine running the program. Lines 34-63 actually read in these fields at their correct offsets and verify that the header's fields match the defined targets. Lines 77-80, 87-89 and 185-188 load e_phoff, e_phnum, and e_entry respectively.

# C — Program Header Table Parsing #
ELFParser.cpp lines 77-80 read in e_phoff, lines 116-118 use e_phoff as a base when seeking to the n'th program header. Line 100 wraps reads in a for loop that iterates over e_phnum entries. Lines 116-124 load and check p_type and skip if it is not loadable. Line 117 also uses e_phentsize instead of a hardcoded value when reading bytes. Lines 126-139 load all program header fields in order.

# D — Segment Loading and Zero-Fill #
ELFParser.cpp lines 126-139 read in program header fields. Line 158 seeks to p_offset and line 159 reads in p_filesz bytes from that offset. Line 167 sets the ram at p_vaddr to the read bytes. Lines 161 to 164 zero fill p_memsz - p_filsz trailing bytes (before ram is set). This is wrapped in a for loop at line 100 that loops over e_phnum segments (handles multiple segments in order). MachineCore.hpp lines 50-59 declare the implementation for loading ram to p_vaddr.

# E — Entry Point #
ELFParser.cpp lines 185-196 load and return e_entry to Machine.cpp (caller) after all the entire ELF is handled and lines 20-23 in Machine.cpp set all Harts to the PC returned (not sure how this will change when multiple Harts are implemented, but should be good for now). 


# F — Robustness and Endianness #
ELFParser.cpp All header and PH fields are bit_casted which read them in as little endian from little endian ELF. Lines 23-25 fatalError if the file is shorter than 52 bytes (and thus the 52 byte header cant exist). Lines 111-113 fatalError if file is shorter than e_phoff + (e_phnum+1) * e_phentsize, meaning there isnt enough bytes to hold the program header we are trying to read. Lines 155-157 fatalError when the file is shorter than the p_offset + p_filesz trying to be read, meaning there isnt enough bytes in the file to hold the bytes we want to load into ram. Lines 53-56 in MachineCore.hpp fatalError if RAM is not large enough to hold bytes being loaded. All bytes are loaded as little-endian from the file, but if the machine hosting the program is big-endian, it will byteswap the internal values so that seeks, reads, etc. are accurate to little-endian representation from the ELF.

