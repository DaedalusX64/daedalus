cmake_minimum_required(VERSION 3.16)
project (DaedalusX64 LANGUAGES C CXX ASM)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

message(${CMAKE_SYSTEM_PROCESSOR} " Detected") 

add_subdirectory(Source)