project (DaedalusX64 LANGUAGES C CXX ASM )
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) 

message("DaedalusX64 Build")
message(${CMAKE_SYSTEM_PROCESSOR} " Detected")

	#Build Type
	if(DEBUG)
		set(CMAKE_BUILD_TYPE Debug)
		add_compile_definitions(DAEDALUS_CONFIG_VERSION="Dev")
		option(DAEDALUS_ENABLE_ASSERTS "Enable Static Asserts" ON)
		option(DAEDALUS_DEBUG_CONSOLE "Enable Debug Console" ON)
		option(DAEDALUS_DEBUG_DISPLAYLIST "Options for debugging the display list, useful for determining texture issues" ON)
		option(DAEDALUS_LOG "Enable various logging" ON)
		add_definitions("-MD")
	else()
	message(${CMAKE_CXX_FLAGS_RELEASE}) ## Need to investigate this further as it breaks macOS build.. See what the PSP has on it for example
		add_compile_definitions(DAEDALUS_CONFIG_VERSION="Release") 
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
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynarec" ON)
	else()
		message("Platform unknown, Dynarec disabled")
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynarec" OFF)
		endif()


	# General options

	option(DAEDALUS_BATCH_TEST_ENABLED "Enable batch testing for compatibility" OFF)
	option(DAEDALUS_ENABLE_OS_HOOKS "Enable High Level Emulation Hooks" ON) ## Per platform?
	option(DAEDALUS_COMPRESSED_ROM_SUPPORT "Enable Compressed ROM Support" OFF) ## Probably should be a default option. 
	option(DAEDALUS_SILENT "Shhh, no debug console message, on by default" OFF)

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

	#Windows -- Not sure if it works yet
	set (WIN_AUDIO SysW32/HLEAudio/AudioPluginW32.cpp)
	set (WIN_DEBUG SysW32/Debug/DaedalusAssertW32.cpp)
	set (WIN_DYNAREC SysW32/DynaRec/x86/AssemblyUtilsX86.cpp SysW32/DynaRec/x86/AssemblyWriterX86.cpp SysW32/DynaRec/x86/CodeBufferManagerX86.cpp SysW32/DynaRec/x86/CodeGeneratorX86.cpp)
	set (WIN_INTERFACE SysW32/Interface/FileNameHandler.cpp)
	set (WIN_MAIN_FILES SysW32/main.cpp)
	set (WIN_UTILITY SysW32/Utility/IOW32.cpp SysW32/Utility/ThreadW32.cpp SysW32/Utility/TimingW32.cpp SysW32/Utility/CondW32.cpp SysW32/Utility/DISASM86.cpp)
	set (WIN_BUILD ${WIN_DYNAREC} ${WIN_AUDIO} ${WIN_DEBUG} ${WIN_INTERFACE} ${WIN_UTILITY})

	#Posix
	# set (POSIX_DYNAREC SysPosix/DynaRec/CodeBufferManagerPosix.cpp)
	# set (POSIX_HLEGRAPHICS SysPosix/HLEGraphics/DisplayListDebugger.cpp)
	#  set (POSIX_MAIN_FILES S)
	# set (POSIX_UTILITY SysPosix/Utility/CondPosix.cpp SysPosix/Utility/IOPosix.cpp SysPosix/Utility/ThreadPosix.cpp SysPosix/Utility/TimingPosix.cpp)
	# set (POSIX_BUILD ${POSIX_DEBUG} ${POSIX_DYNAREC} ${POSIX_HLEGRAPHICS} ${POSIX_UTILITY})

	# These will remain separate for now..
	# set (LINUX_AUDIO SysPosix/HLEAudio/AudioPluginLinux.cpp)
	# set (MAC_AUDIO SysPosix/HLEAudio/AudioPluginOSX.cpp)


	if (PSP)
		set (PLAT PSP)
		add_definitions(-DDAEDALUS_PSP)
		# add_compile_definitions(DAEDALUS_DIALOGS)

		include_directories(BEFORE ${PROJECT_SOURCE_DIR}/SysPSP/Include .)
		set(PLAT_LIBS psppower pspgu pspctrl pspdebug pspsdk pspdisplay pspge -lpspaudiolib -lpspaudio -lintrafont -lpspkubridge -lpspfpu)

	##Universal build options - Generic
 	add_compile_definitions(${PLAT}) 
	# add_library(daedalus.lib STATIC ${BUILD} ${${PLAT}_BUILD})
	add_executable(daedalus SysPSP/main.cpp)

	## No idea why this fails on the PSP Build
	target_link_libraries(daedalus LINK_PUBLIC
												Debug_PSP DynaRec_PSP Graphics_PSP HLEAudio_PSP HLEGraphics_PSP Input_PSP Plugins_PSP UI_PSP Utility_PSP
												Core Config Debug Graphics HLEAudio HLEGraphics Interface Math OSHLE RomFile System Utility
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

	if (LINUX)
		add_definitions(-DDAEDALUS_LINUX)
		## Assume Linux uses X11 for now
		set (OS_LIBS X11 GL)
		set (AUDIO_DRIVER ${LINUX_AUDIO})
	endif (LINUX)

	if (APPLE)
		set (OS_LIBS "-framework OpenGL -framework AudioToolbox -framework Cocoa -framework IOKit -framework CoreVideo")
		set (AUDIO_DRIVER ${MAC_AUDIO})

		## Homebrew uses a separate path on Apple Silicon
		if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64" ) 
		include_directories(/opt/homebrew/include) 
		link_directories(/opt/homebrew/lib)
		else()
		include_directories(/usr/include/include)
		link_directories(/usr/include/lib)
		endif()
	endif (APPLE)

	if (UNIX)
		add_definitions(-DDAEDALUS_POSIX -DDAEDALUS_GL)
		 include_directories(BEFORE ${PROJECT_SOURCE_DIR}/SysPosix/Include ${PROJECT_SOURCE_DIR})

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
		include_directories(BEFORE ${PROJECT_SOURCE_DIR}/third_party/Libs/glew-2.1.0/include)
		include_directories(BEFORE ${PROJECT_SOURCE_DIR}/third_party/Libs/SDL2-2.0.12/include)
		include_directories(BEFORE ${PROJECT_SOURCE_DIR}/third_party/Libs/LibPNG/include)
		include_directories(BEFORE ${PROJECT_SOURCE_DIR}/third_party/Libs/Zlib/include)
		include_directories(BEFORE ${PROJECT_SOURCE_DIR}/third_party/Libs/Minizip)
		include_directories(BEFORE ${PROJECT_SOURCE_DIR}/SysW32/Include ${PROJECT_SOURCE_DIR})
		link_directories(${PROJECT_SOURCE_DIR}/third_party/Libs/Lib)

		add_library(sysGL STATIC ${SYSGL_BUILD})
		target_link_libraries(sysGL OpenGL32 glfw3 glew32s SDL2 Xinput ${OS_LIBS})

		add_library(daedalus.lib STATIC ${BUILD} ${WIN_BUILD} ${AUDIO_DRIVER} )
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
	else(PSP)
	add_subdirectory(SysPSP)
	endif()