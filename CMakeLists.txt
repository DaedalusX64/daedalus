cmake_minimum_required(VERSION 3.16)
project (DaedalusX64 LANGUAGES C CXX ASM)

message(${CMAKE_SYSTEM_PROCESSOR} " Detected") 

INCLUDE(FindPkgConfig)

add_subdirectory(Source)