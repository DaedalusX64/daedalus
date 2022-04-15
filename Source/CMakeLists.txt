project (DaedalusX64 LANGUAGES C CXX ASM )
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) 

message("DaedalusX64 Build")
message(${CMAKE_SYSTEM_PROCESSOR} " Detected") 

	#Build Type
	if(DEBUG)
		set(CMAKE_BUILD_TYPE Debug)
		add_compile_definitions(DAEDALUS_CONFIG_VERSION="Dev") ## Perhaps we can remove this. It's only used in the PSP UI and model doesn't matter as much
		option(DAEDALUS_ENABLE_ASSERTS "Enable Static Asserts" ON)
		option(DAEDALUS_DEBUG_CONSOLE "Enable Debug Console" ON)
		option(DAEDALUS_DEBUG_DISPLAYLIST "Options for debugging the display list, useful for determining texture issues" ON)
		option(DAEDALUS_LOG "Enable various logging" ON)
		add_definitions("-MD")
	else()
	message(${CMAKE_CXX_FLAGS_RELEASE}) ## Need to investigate this further as it breaks macOS build.. See what the PSP has on it for example
		add_compile_definitions(DAEDALUS_CONFIG_VERSION="Release") ## Perhaps we can remove this. It's only used in the PSP UI and model doesn't matter as much
		add_compile_definitions(fltfo)
		add_compile_definitions(NDEBUG)
	endif()

	##Determine Dynarec compatibility and link in appropriate Dynarec
	##Todo Move all relevant Dyanrecs to the Dynarec/arch folder and enable here when correct platform is detected
	if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64" )
		option(DAEDALUS_ENABLE_DYNAREC  "Enable Dynarec" OFF)
		message("No Dynarec on arm64 yet")
	elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "mips" )
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynarec" ON)
	elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86")
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynarec" OFF)
		## X86 Dynarec Code needs to be moved from W32 emulator
	elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm")
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynarec" ON)
		add_subdirectory(DynaRec/ARM)
	else()
		message("Platform unknown, Dynarec disabled")
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynarec" OFF)
		endif()


	# General options

	option(DAEDALUS_BATCH_TEST_ENABLED "Enable batch testing for compatibility" OFF)
	option(DAEDALUS_ENABLE_OS_HOOKS "Enable High Level Emulation Hooks" ON) ## Per platform?
	option(DAEDALUS_COMPRESSED_ROM_SUPPORT "Enable Compressed ROM Support" OFF) ## Probably should be a default option. 
	option(DAEDALUS_SILENT "Shhh, no debug console message, on by default" ON)

	# Additional Debug Options
	option(DAEDALUS_DEBUG_MEMORY "Debug memory issues" OFF)
	option(DAEDALUS_DEBUG_PIF "Enable Debugging Options for Peripheral Interface (PIF)" OFF)
	option(DAEDALUS_DEBUG_DYNAREC "Enable Various Debugging options for the Dynarec" OFF)
	option(DAEDALUS_ENABLE_SYNCHRONISATION "Enable for sync testing" OFF)
	option(DAEDALUS_ENABLE_PROFILING "Enable built in profiler" OFF)
	option(DAEDALUS_PROFILE_EXECUTION "Keep track of various execution stats" OFF)

	# Additional PSP Options
	option(DAEDALUS_PSP_GPROF "PSP Profiling, needs Silent enabled" OFF)

	## Enable specific options per platform based on Release build ## XXX need to handle GPROF

	if(PSP) 
	option(DAEDALUS_DIALOGS "Enable UI Dialogs" ON)
	option(DAEDALUS_ENABLE_DYNAREC "Enable Dynamic Recompilation, Disable if system not supported" ON)
	option(DAEDALUS_PSP "PSP specific Options" ON)
	option(DAEDALUS_PSP_USE_ME "Use the Media Engine" ON)
	option(DAEDALUS_PSP_USE_VFPU "Enable VFPU Instructions" ON)
	elseif(UNIX)
	option(DAEDALUS_ENABLE_DYNAREC "Enable Dynamic Recompilation, Disable if system not supported" OFF)
	option(DAEDALUS_POSIX "Posix specific options" ON)
	option(DAEDALUS_ACCURATE_TMEM "Accurate TMEM" ON)
	endif()

	include(${PROJECT_SOURCE_DIR}/Options.cmake)
	
	#Endianness Check
	set(DAEDALUS_ENDIAN_LITTLE 1)
	set(DAEDALUS_ENDIAN_BIG 2)

	include (TestBigEndian)
	TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
	if(IS_BIG_ENDIAN)
	message(STATUS "Big Endian Detected")
	set(ENDIAN DAEDALUS_ENDIAN_BIG)
	add_compile_definitions(DAEDALUS_ENDIAN_MODE DAEDALUS_ENDIAN_BIG)
	else()
	message(STATUS "Little Endian Detected")
	add_compile_definitions(DAEDALUS_ENDIAN_MODE DAEDALUS_ENDIAN_LITTLE)
	endif()

	## These build options are required so we define as nothing until needed
	if(NOT WIN32)
		add_compile_definitions(R4300_CALL_TYPE=${})
		add_compile_definitions(DAEDALUS_THREAD_CALL_TYPE=${})
		add_compile_definitions(DAEDALUS_VARARG_CALL_TYPE=${})
		add_compile_definitions(DAEDALUS_ZLIB_CALL_TYPE=${})
	endif()

## These are only defined in the PSP Build
	if(NOT PSP)
		add_compile_definitions(DAEDALUS_ATTRIBUTE_PURE=${}) #Does not modify global memory
		add_compile_definitions(DAEDALUS_ATTRIBUTE_CONST=${}) #Does not read/modify global memory
	endif()

	#PSP Build
	set (GPROF_SRCS SysPSP/Debug/prof.c SysPSP/Debug/mcount.S )
	set (PSP_MAIN_FILES SysPSP/main.cpp)

	if (PSP)
		add_definitions(-DDAEDALUS_PSP)
		include_directories(BEFORE ${PROJECT_SOURCE_DIR}/SysPSP/Include .)
		set(PLAT_LIBS psppower pspgu pspctrl pspdebug pspsdk pspdisplay pspge -lpspaudiolib -lpspaudio -lintrafont -lpspkubridge -lpspfpu -lpspge)

	add_executable(daedalus SysPSP/main.cpp)

	## No idea why this fails on the PSP Build
	target_link_libraries(daedalus LINK_PUBLIC
												Debug_PSP DynaRec_PSP Graphics_PSP HLEAudio_PSP HLEGraphics_PSP Input_PSP Plugins_PSP UI_PSP Utility_PSP
												Core Config Debug DynaRec Graphics HLEAudio HLEGraphics Interface Math OSHLE RomFile System Utility
												stdc++ png z c g m pthread ${PLAT_LIBS} )
 
	create_pbp_file(
    	TARGET daedalus
    	ICON_PATH ${PROJECT_SOURCE_DIR}/SysPSP/Resources/eboot_icons/icon0.png
    	BACKGROUND_PATH ${PROJECT_SOURCE_DIR}/SysPSP/Resources/eboot_icons/pic1.png
    	PREVIEW_PATH NULL
    	TITLE DaedalusX64
    	BUILD_PRX
				)
 	endif (PSP)

	if (UNIX AND NOT APPLE)
		add_definitions(-DDAEDALUS_LINUX) ## Still required?
		## Assume Linux uses X11 for now
		set (OS_LIBS X11 GL)
		set (AUDIO_DRIVER ${LINUX_AUDIO})
	endif (UNIX AND NOT APPLE)

	if (APPLE)
		set (OS_LIBS "-framework OpenGL -framework AudioToolbox -framework Cocoa -framework IOKit -framework CoreVideo")
		set (AUDIO_DRIVER ${MAC_AUDIO})

		## Homebrew uses a separate path on Apple Silicon
		if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64" ) 
		include_directories(/opt/homebrew/include) 
		link_directories(/opt/homebrew/lib)
		else()
		include_directories(/usr/local/include)
		link_directories(/usr/local/lib)
		endif()
	endif (APPLE)

	if (UNIX)
		add_definitions(-DDAEDALUS_POSIX -DDAEDALUS_GL)
		 include_directories(BEFORE ${PROJECT_SOURCE_DIR}/SysPosix/Include .)

		add_executable(daedalus SysPosix/main.cpp)

		target_link_libraries(daedalus LINK_PUBLIC
												   Core Config Debug Graphics HLEAudio HLEGraphics Interface Math OSHLE RomFile System Utility 
												   Graphics_GL HLEGraphics_GL Input_GL Interface_GL 
												   Posix_Debug Posix_DynaRec Posix_Audio Posix_HLEGraphics Posix_Utility  
												   png z minizip pthread GLEW SDL2 dl ${OS_LIBS}
		)
	endif (UNIX)

	if(WIN32)
		add_definitions(-DDAEDALUS_WIN32 -DDAEDALUS_GL)
		#add_compile_options("/Od")
		##Windows todo
		# Pull in modules for glew / SDL2 / ZLib / LibPNG / Minizip
		include_directories(BEFORE ${PROJECT_SOURCE_DIR}/SysW32/Include)

		add_library(sysGL OBJECT ${SYSGL_BUILD})
		target_link_libraries(sysGL OpenGL32 glfw3 glew32s SDL2 Xinput ${OS_LIBS})

		add_library(daedalus.lib OBJECT ${BUILD} ${WIN_BUILD} ${AUDIO_DRIVER} )
		target_link_libraries(daedalus.lib dsound dxguid sysGL libpng zlib zlibwapi)

		#Build and Link Executable
		add_executable(daedalus ${WIN_MAIN_FILES})
		target_link_libraries(daedalus LINK_PUBLIC daedalus.lib Shlwapi.lib)


	endif (WIN32)

	## Daedalus Core Build
	add_subdirectory(Config)
	add_subdirectory(Core)
	add_subdirectory(Debug)
	add_subdirectory(DynaRec)
	add_subdirectory(Graphics)
	add_subdirectory(HLEAudio)
	add_subdirectory(HLEGraphics)
	add_subdirectory(Interface)
	add_subdirectory(Math)
	add_subdirectory(OSHLE)
	add_subdirectory(RomFile)
	add_subdirectory(System)
	add_subdirectory(Test)
	add_subdirectory(Utility)

	## Daedalus GL -- Need to if / else this
	if(UNIX)
	add_subdirectory(SysPosix)
	add_subdirectory(SysGL)
	elseif(WIN) ## Probably keep this generic as we'll build this on Linux too?
	add_subdirectory(SysW32)
	elseif(CTR)
	add_subdirectory(SysCTR)
	else(PSP)
	add_subdirectory(SysPSP)
	endif()