cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Release)
project (DaedalusX64 LANGUAGES C CXX ASM)

if(UNIX)
include_directories(Source/SysPosix/Include)
elseif(PSP)
include_directories(Source/SysPSP/Include)
elseif(WIN32)
include_directories(Source/SysW32/Include)
endif(UNIX)

## Deal with endianness
include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
	if(IS_BIG_ENDIAN)
	add_compile_definitions(DAEDALUS_ENDIAN_MODE=DAEDALUS_ENDIAN_BIG)
	else()
	add_compile_definitions(DAEDALUS_ENDIAN_MODE=DAEDALUS_ENDIAN_LITTLE)
endif(IS_BIG_ENDIAN)

#General
option(DAEDALUS_ENABLE_DYNAREC "Enable Dynamic Recompiler" OFF)
option(DAEDALUS_COMPRESSED_ROM_SUPPORT "Enable Compressed ROM Support" OFF)
option(DAEDALUS_ENABLE_OS_HOOKS "Enable OS Hooks" OFF)
option(DAEDALUS_DIALOGS "Enable Confirmation Dialogs in the GUI" OFF)
option(DAEDALUS_SILENT "Silence the console" OFF)
#Debug
option(DAEDALUS_DEBUG_CONSOLE "Enable Debug Console" OFF)
option(DAEDALUS_DEBUG_DISPLAYLIST "Enable DisplayList Debugger" ON)
option(DAEDALUS_DEBUG_MEMORY "Debug Memory" OFF)
option(DAEDALUS_DEBUG_DYNAREC "Debug Dynamic Recompiler" OFF)
option(DAEDALUS_DEBUG_PIF "Debug Peripheral Interface" OFF)
option(DAEDALUS_ENABLE_SYNCHRONISATION "Enable for sync testing" OFF)
option(DAEDALUS_ENABLE_ASSERTS "Enable Asserts" OFF)
option(DAEDALUS_ENABLE_PROFILING "Enable built in profiler" OFF)
option(DAEDALUS_PROFILE_EXECUTION "Keep Track of various execution stats" OFF)
option(DAEDALUS_BATCH_TEST_ENABLED "Enable Batch Test Mode" OFF)
option(ALLOW_TRACES_WHICH_EXCEPT "Allow Traces which except" OFF)
option(DAEDALUS_LOG "Enable various logging" OFF)
option(DAEDALUS_BREAKPOINTS_ENABLED "Enable Breakpoints" OFF)


if(DEBUG)
	message("Debug Build")
	include_directories(Source)
	add_compile_options("-g -G0 -D_DEBUG -MD ")
	add_compile_definitions(
	DAEDALUS_DEBUG_CONSOLE
	DAEDALUS_DEBUG_DISPLAYLIST
	DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_BATCH_TEST_ENABLED
	DAEDALUS_LOG
	DAEDALUS_CONFIG_VERSION="Dev"
							)
elseif(PROFILE)
	message("PSP GPROF Build")
	include_directories(Source)
	add_compile_definitions(
							DAEDALUS_SILENT
							DAEDALUS_PSP_GPROF
							DAEDALUS_CONFIG_VERSION="PSP GPROF"
							)
else()
	message("Release Build")
	include_directories(Source)
	add_definitions("-O3")
	add_compile_definitions(
							DAEDALUS_DIALOGS
							DAEDALUS_SILENT
							DAEDALUS_CONFIG_VERSION="Release"
							)		
endif(DEBUG)

## Use CMake Detection for OSs that are defined Platforms
## Todo add CPU check for ARM
if(WIN32)
	add_compile_definitions(
		DAEDALUS_W32
		DAEDALUS_ENABLE_DYNAREC
		DAEDALUS_ENABLE_OS_HOOKS
		DAEDALUS_COMPRESSED_ROM_SUPPORT
		DAEDALUS_ACCURATE_TMEM	
	)
	add_subdirectory(Source/third_party)
	add_subdirectory(Source/SysW32)
elseif(UNIX)
	add_compile_definitions(	DAEDALUS_POSIX 
								DAEDALUS_GL
							)
	add_subdirectory(Source/SysGL)
	add_subdirectory(Source/SysPosix)
	add_subdirectory(Source/third_party)
elseif(PSP)
	add_compile_definitions(
							DAEDALUS_PSP
							DAEDALUS_ENABLE_DYNAREC
							DAEDALUS_PSP_USE_ME
							DAEDALUS_ENABLE_OS_HOOKS
							DAEDALUS_PSP_USE_VFPU
							)
	include_directories(Source/SysPSP/Include)
	add_subdirectory(Source/SysPSP)
elseif(VITA)
#	add_subdirectory(Source/SysVita)
else(CITRA)
	#add_subdirectory(Source/SysCitra)
endif(WIN32)

add_subdirectory(Source)