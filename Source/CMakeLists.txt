# DaedalusX64 CMake Build File 
# Created and Maintained by Wally 
# Yes right now this is a mess, hoping to simplify things further in future but please deal with it for now :)

set(CMAKE_CXX_STANDARD 20)

# Below is related to Macros within the project, a lot of these could be deprecated and deleted.
# For now they will remain here so they're visible and can be switched on and off as desired.

#TODO: Separate SDL Options
list(APPEND plat_main )

## if DEBUG is added to the build_daedalus script it will execute teh Debug section otherwise by default do release.
if(DEBUG)
	set(CMAKE_BUILD_TYPE Debug)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
	add_compile_definitions(DAEDALUS_CONFIG_VERSION="Dev") ## Perhaps we can remove this. It's only used in the PSP UI and model doesn't matter as much
	option(DAEDALUS_ENABLE_ASSERTS "Enable Static Asserts" ON)
	option(DAEDALUS_DEBUG_CONSOLE "Enable Debug Console" ON)
	option(DAEDALUS_DEBUG_DISPLAYLIST "Options for debugging the display list, useful for determining texture issues" OFF)
	option(DAEDALUS_LOG "Enable various logging" ON)
	add_compile_options("-Wall")
else()
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
	add_compile_definitions(DAEDALUS_CONFIG_VERSION="Release") ## Perhaps we can remove this. It's only used in the PSP UI and model doesn't matter as much
endif()
	#Determine Dynarec compatibility and link in appropriate Dynarec
	#Todo: Move all relevant Dynarec to the Dynarec/arch folder and enable here when correct platform is detected
	if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64" )
		option(DAEDALUS_ENABLE_DYNAREC  "Enable Dynarec" OFF)
		message("No Dynarec on arm64 yet")
	elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "mips" )
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynarec" ON)
		set(DYNAREC DynaRec) #Enable Dynarec Linker Flag
	elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86")
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynarec" OFF)
		## X86 Dynarec Code needs to be moved from W32 emulator
	elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "armv")
		message("ARM Dynarec Enabled")
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynarec" ON)
		set(DYNAREC DynaRec) #Enable Dynarec Linker Flag
	elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
		message("x86_64 Dynarec Enabled")
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynarec" ON)
	else()
		message("Platform unknown, Dynarec disabled")
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynarec" OFF)
	endif()
	
	#Check the Endian of the host platform and apply endian rules. 
	include (TestBigEndian)
	TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
	if(IS_BIG_ENDIAN)
		message(STATUS "Big Endian Detected")
		option(DAEDALUS_ENDIAN_BIG "Big Endian Processor" ON)
	else()
		message(STATUS "Little Endian Detected")
		option(DAEDALUS_ENDIAN_LITTLE "Litle Endian Processor" ON)
	endif()

	# General options

	option(DAEDALUS_BATCH_TEST_ENABLED "Enable batch testing for compatibility" OFF)
	option(DAEDALUS_ENABLE_OS_HOOKS "Enable High Level Emulation Hooks" ON) ## Per platform?
	option(DAEDALUS_COMPRESSED_ROM_SUPPORT "Enable Compressed ROM `Support" OFF) ## Probably should be a default option. 
	option(DAEDALUS_SILENT "Shhh, no debug console message, on by default" ON)
	option(DAEDALUS_LOG "Log stuff " OFF) ## NEeds to be off on the PSP unless debugging

	# Additional Debug Options
	option(SHOW_MEM "Show memory usage" OFF)
	option(DAEDALUS_DEBUG_MEMORY "Debug memory issues" OFF)
	option(DAEDALUS_DEBUG_PIF "Enable Debugging Options for Peripheral Interface (PIF)" OFF)
	option(DAEDALUS_DEBUG_DYNAREC "Enable Various Debugging options for the Dynarec" OFF)
	option(DAEDALUS_ENABLE_SYNCHRONISATION "Enable for sync testing" OFF)
	option(DAEDALUS_ENABLE_PROFILING "Enable built in profiler" OFF)
	option(DAEDALUS_PROFILE_EXECUTION "Keep track of various execution stats" OFF)
	option(DUMPOSFUNCTIONS "Dump OSHLE Functions" OFF)
	# Additional PSP Options
	option(DAEDALUS_PSP_GPROF "PSP Profiling, needs Silent enabled" OFF)


	## Enable specific options per platform based on Release build ## XXX need to handle GPROF
	# Handle specific options (These were all found in platforms.h and now belong here)
	# PSP / Vita / CTR options are required as non-linux embedded platforms are not able to be reliably detected.
	# Windows, Linux and macOS builds are auto detected.


	if(PSP) 
		message("PSP")
		option(DAEDALUS_DIALOGS "Enable UI Dialogs" ON)
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynamic Recompilation, Disable if system not supported" ON)
		option(DAEDALUS_PSP "PSP specific Options" ON)
		option(DAEDALUS_PSP_USE_ME "Use the Media Engine" ON)
		option(DAEDALUS_PSP_USE_VFPU "Enable VFPU Instructions" ON)
		option(DAEDALUS_SIM_DOUBLES "Simulate Doubles" ON)
		option(DAEDALUS_SDL "Enable SDL" ON)
	elseif(UNIX)
		option(DAEDALUS_SDL "SDL Build" ON)
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynamic Recompilation, Disable if system not supported" OFF)
		option(DAEDALUS_POSIX "Posix specific options" ON)
		option(DAEDALUS_ACCURATE_TMEM "Accurate TMEM" ON)
		option(DAEDALUS_128BIT_MULT "128 Bit Multiplication, not supported on PSP" ON)
		option(DAEDALUS_DAEDALUS_ACCURATE_CVT "Convert using the rounding mode specified in the Floating Control/Status register (FCSR)")
		option(DAEDALUS_GL "OpenGL Renderer" ON)
	elseif(WIN32)
		option(DAEDALUS_WIN32 "Windows specific Options" ON)
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynamic Recompilation, Disable if system not supported" ON)
		option(DAEDALUS_ACCURATE_TMEM "Accurate TMEM" ON)
		option(DAEDALUS_BREAKPOINTS_ENABLED "Enable Breakpoints" OFF)
		option(DAEDALUS_GL "OpenGL Renderer" ON)
	elseif(CTR)
		option(DAEDALUS_CTR "3DS Specific Options" ON)
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynamic Recompilation" ON)
		option(DAEDALUS_ENABLE_OS_HOOKS " High Level Emulation Hooks" ON)
	endif()

	if(DAEDALUS_SDL)
		find_package(SDL2 CONFIG REQUIRED)
		list(APPEND default_libraries "${SDL2_LIBRARIES}")
		list(APPEND include_dirs "${SDL2_INCLUDE_DIRS}")
	endif(DAEDALUS_SDL)

	# Needed below the options to post process
	include(${PROJECT_SOURCE_DIR}/Options.cmake) 

	#Setup directories to include
	list(APPEND include_dirs "${PROJECT_SOURCE_DIR}/Source")
	include_directories( BEFORE ${include_dirs})


## Generic defines for certain things that Windows requires.
if(NOT WIN32)
	add_compile_definitions(DAEDALUS_THREAD_CALL_TYPE=${})
	add_compile_definitions(DAEDALUS_VARARG_CALL_TYPE=${})
	add_compile_options("-g")
	add_compile_options("-ffast-math")
endif()

if(UNIX)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
endif(UNIX)

if (APPLE)
	list(APPEND sys_libraries "-framework OpenGL -framework AudioToolbox -framework Cocoa -framework IOKit -framework CoreVideo")
	## Homebrew uses a separate path on Apple Silicon
	##Ugh is there a better way??
	if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64" )
		include_directories(/opt/homebrew/include)
		link_directories(/opt/homebrew/lib)
	else()
		include_directories(/usr/local/include)
		link_directories(/usr/local/lib)
	endif()
endif (APPLE)
if(CTR)
	include_directories($ENV{DEVKITPRO}/portlibs/3ds/include)
	include_directories($ENV{DEVKITPRO}/picaGL/include)
	link_directories($ENV{DEVKITPRO}/picaGL/lib)
	include_directories($ENV{DEVKITPRO}/portlibs/3ds/lib)
endif(CTR)

if(WIN32)
	set(CMAKE_TOOLCHAIN_FILE ${VCPKG_TOOLCHAIN})
	find_package(ZLIB)
	find_package(SDL2 CONFIG REQUIRED)
	find_package(GLEW REQUIRED)
		
	set(sys_libraries png z minizip pthread GLEW SDL2 dl ${OS_LIBS})
	# Pull in modules for glew / SDL2 / ZLib / LibPNG / Minizip

#	add_library(sysGL OBJECT ${SYSGL_BUILD})
#	target_link_libraries(sysGL OpenGL32 glfw3 glew32s SDL2 Xinput ${OS_LIBS})

#	add_library(daedalus.lib OBJECT ${BUILD} ${WIN_BUILD} ${AUDIO_DRIVER} )
#	target_link_libraries(daedalus.lib dsound dxguid sysGL libpng zlib zlibwapi)

	#Build and Link Executable

	add_executable(daedalus SysW32/main.cpp)
	target_link_libraries(daedalus LINK_PUBLIC
				   Core Config Debug Graphics HLEAudio HLEGraphics Interface Math OSHLE RomFile System Utility
				   Graphics_GL HLEGraphics_GL Input_GL Interface_GL imgui
				   Win_Debug Win_Audio Win_Utility)
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
		add_subdirectory(UI)

	if(UNIX)
		add_subdirectory(SysPosix)
		add_subdirectory(SysGL)
	elseif(WIN32) ## Probably keep this generic as we'll build this on Linux too?
		add_subdirectory(SysGL)
		add_subdirectory(SysW32)
	elseif(CTR)
		add_subdirectory(SysCTR)
	elseif(PSP)
		add_subdirectory(SysPSP)
	endif()


	# ---  Build ---- 
	add_executable(daedalus ${plat_main})
	list(APPEND default_libraries png z) ## Required Libraries 
	
	message(" ${daed_libs} ${default_libraries} ${sys_libraries} " )
	target_include_directories(daedalus BEFORE PUBLIC ${include_dirs})
	target_link_libraries(daedalus PRIVATE ${daed_libs} ${default_libraries} ${sys_libraries})

	# --- Post Build ---
	if(PSP)
		create_pbp_file(
			TARGET daedalus
			ICON_PATH ${PROJECT_SOURCE_DIR}/Source/SysPSP/Resources/eboot_icons/icon0.png
			BACKGROUND_PATH ${PROJECT_SOURCE_DIR}/Source/SysPSP/Resources/eboot_icons/pic1.png
			PREVIEW_PATH NULL
			TITLE DaedalusX64
			)
	endif(PSP)

	if(CTR)
	set(APP_TITLE "DaedalusX64")
	set(APP_DESCRIPTION "DaedalusX64 port for 3DS")
	set(APP_AUTHOR "MasterFeizz / DaedalusX64 Team")

	set(APP_SMDH "${PROJECT_SOURCE_DIR}/SysCTR/Resources/daedalus.smdh")
	set(APP_BANNER "${PROJECT_SOURCE_DIR}/SysCTR/Resources/banner.bnr")
	set(APP_AUDIO "${PROJECT_SOURCE_DIR}/SysCTR/Resources/audio_silent.wav")
	set(APP_RSF "${PROJECT_SOURCE_DIR}/SysCTR/Resources/template.rsf")
	set(APP_ROMFS_DIR "${PROJECT_SOURCE_DIR}/SysCTR/Resources/RomFS")
	set(APP_ROMFS_BIN "${PROJECT_SOURCE_DIR}/SysCTR/Resources/romfs.bin")

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/DaedalusX64.3dsx
		COMMAND 3dsxtool daedalus.elf DaedalusX64.3dsx --romfs=${APP_ROMFS_DIR} --smdh=${APP_SMDH}
		DEPENDS daedalus.elf
	)
	
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/DaedalusX64.cia
		COMMAND makerom -f cia
			-target t
			-exefslogo
			-o ${CMAKE_CURRENT_BINARY_DIR}/DaedalusX64.cia
			-elf daedalus.elf
			-rsf ${APP_RSF}
			-banner ${APP_BANNER}
			-icon ${APP_SMDH}
			-romfs ${APP_ROMFS_BIN}
		DEPENDS daedalus.elf ${APP_RSF}
	)
	
	#  add_custom_target(N3DS ALL
	#  	DEPENDS Daedalus.elf
	# 			DaedalusX64.3dsx
	#  			DaedalusX64.cia
	#  )
	
	
	endif(CTR)
	
	## Install and setup basic directories
	install(DIRECTORY DESTINATION DaedalusX64)
	install(DIRECTORY DESTINATION DaedalusX64/Roms)
	install(DIRECTORY DESTINATION DaedalusX64/SaveGames)
	install(DIRECTORY DESTINATION DaedalusX64/SaveStates)
