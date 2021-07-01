cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Release)
project (DaedalusX64 LANGUAGES C CXX ASM)

## Needs to be better
include_directories(BEFORE Source/SysPosix/Include)
## include_directories(BEFORE Source/SysPSP/Include)
if(DEBUG)
	message("Debug Build")
	include_directories(Source Source/Base/Dev)
	add_definitions("-g -G0 -D_DEBUG -MD ")
elseif(PROFILE)
	message("Profile Build")
	include_directories(Source Source/Base/Profile)
else()
	message("Release Build")
	include_directories(Source)
	add_definitions("-O2 -DNDEBUG")
    include_directories(Source Source/Base/Release)
endif(DEBUG)

add_subdirectory(Source)

## Use CMake Detection for OSs that are defined Platforms
## Todo add CPU check for ARM
if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
	add_subdirectory(Source/third_party)
	add_subdirectory(Source/SysW32)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL Linux OR Darwin)
	add_subdirectory(Source/third_party)
	add_subdirectory(Source/SysGL)
	add_subdirectory(Source/SysPosix)
endif(${CMAKE_SYSTEM_NAME} STREQUAL Windows)

## Define other platforms here.
if(PSP)
add_subdirectory(Source/SysPSP)
elseif(VITA)
add_subdirectory(Source/SysVita)
elseif(CITRA)
add_subdirectory(Source/SysCitra)
endif(PSP)