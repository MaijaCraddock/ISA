cmake_minimum_required(VERSION 3.20)
project(machine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(machine
    src/Machine.cpp

    src/HelperFunctions.hpp
    src/HelperFunctions.cpp

    src/ConfigParser.hpp
    src/ConfigParser.cpp

    src/SignExtend.hpp
    src/SignExtend.cpp

    src/RegisterFile.hpp
    src/RegisterFile.cpp

    src/PC.hpp
    src/PC.cpp

    src/CSR.hpp
    src/CSR.cpp

    src/Hart.hpp
    src/Hart.cpp

    src/MachineCore.hpp
    src/MachineCore.cpp

    src/ELFParser.hpp
    src/ELFParser.cpp

    src/IMemorySeam.hpp

    src/RAM.hpp
    src/RAM.cpp

    src/BranchPredictor.hpp
    src/BranchPredictor.cpp

    src/Fetch.hpp
    src/Fetch.cpp

    src/Decode.hpp
    src/Decode.cpp

    src/Execute.hpp
    src/Execute.cpp
)

target_include_directories(machine PRIVATE src)


if (MSVC)
    target_compile_options(machine PRIVATE
        /W4 /permissive- /utf-8 /MP
        /w14242 /w14254 /w14263 /w14265 /w14287 /w14296 /w14311 /w14545 /w14546 /w14547 /w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928
    )
else()
    target_compile_options(machine PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
        -Wold-style-cast -Wuseless-cast -Wcast-qual
    )
endif()