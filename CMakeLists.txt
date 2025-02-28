#Base Config for CMake
#Last updated by Wally - 28/02/2025

#Basic CMake Configuration info goes here to keep next file mostly project specific.

cmake_minimum_required(VERSION 3.20)
project (DaedalusX64 LANGUAGES C CXX ASM)

#We use std::fmt and that requires C++ 23. Provide an alternative if GCC is older than version 13.
# This does not cater for Clang as clang is often available independently of the OS.
set(CMAKE_CXX_STANDARD 23)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    	message(STATUS "GCC detected: ${CMAKE_CXX_COMPILER_VERSION}")
    
    	if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0")
        	message(STATUS "GCC version is older than 13. Using LibFormat.")
        	message("GCC_LEGACY=ON")
    		option(GCC_LEGACY "Use LibFormat" ON)
    	endif()
endif()

message("Processor Type: " ${CMAKE_SYSTEM_PROCESSOR} " Detected") 

# We take control of the CXX Flags and set Optimisations and options here.


message("Cmake Build Type: " ${CMAKE_BUILD_TYPE})
message("Initial CXX Flags" ${CMAKE_CXX_FLAGS})
message("Initial CXX Release Flags: " ${CMAKE_CXX_FLAGS_RELEASE})
message("Initial CXX Debug Flags: " ${CMAKE_CXX_FLAGS_DEBUG})


if(CMAKE_BUILD_TYPE STREQUAL "Release")

message("Initial Release CXX Flags:" ${CMAKE_CXX_FLAGS_RELEASE})
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
message("Applied CXX Flags:" ${CMAKE_CXX_FLAGS_RELEASE})

else(CMAKE_BUILD_TYPE STREQUAL "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O3")
endif()



INCLUDE(FindPkgConfig)

add_subdirectory(Source)