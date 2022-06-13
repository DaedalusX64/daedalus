set(CMAKE_FIND_NO_INSTALL_PREFIX TRUE)

project (DaedalusX64 LANGUAGES C CXX ASM)
cmake_minimum_required(VERSION 3.16)

message(${CMAKE_SYSTEM_PROCESSOR} " Detected") 

add_subdirectory(Source)