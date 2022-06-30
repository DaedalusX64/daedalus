# DaedalusX64 CMake Build File 
# Created and Maintained by Wally 
# Yes right now this is a mess, hoping to simplify things further in future but please deal with it for now :)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) 

# Below is related to Macros within the project, a lot of these could be deprecated and deleted.
# For now they will remain here so they're visible and can be switched on and off as desired.

## if DEBUG is added to the build_daedalus script it will execute teh Debug section otherwise by default do release.
	if(DEBUG)
		set(CMAKE_BUILD_TYPE Debug)
		add_compile_definitions(DAEDALUS_CONFIG_VERSION="Dev") ## Perhaps we can remove this. It's only used in the PSP UI and model doesn't matter as much
		option(DAEDALUS_ENABLE_ASSERTS "Enable Static Asserts" ON)
		option(DAEDALUS_DEBUG_CONSOLE "Enable Debug Console" ON)
		option(DAEDALUS_DEBUG_DISPLAYLIST "Options for debugging the display list, useful for determining texture issues" ON)
		option(DAEDALUS_LOG "Enable various logging" ON)
		add_definitions("-MD")
		add_definitions("-g")
	else()
	message(${CMAKE_CXX_FLAGS_RELEASE}) ## Need to investigate this further as it breaks macOS build.. See what the PSP has on it for example
		add_compile_definitions(DAEDALUS_CONFIG_VERSION="Release") ## Perhaps we can remove this. It's only used in the PSP UI and model doesn't matter as much
		add_compile_definitions(fltfo)
		add_compile_definitions(NDEBUG)
		add_compile_definitions(O3)
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
	elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm")
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynarec" ON)
		set(DYNAREC DynaRec) #Enable Dynarec Linker Flag
		add_subdirectory(DynaRec/ARM)
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
	# Options below are general macros found within the project, as per earlier, a lot of these may
	# disappear in future as we progress and strip them from the binary if not needed.

	# General options

	option(DAEDALUS_BATCH_TEST_ENABLED "Enable batch testing for compatibility" OFF)
	option(DAEDALUS_ENABLE_OS_HOOKS "Enable High Level Emulation Hooks" ON) ## Per platform?
	option(DAEDALUS_COMPRESSED_ROM_SUPPORT "Enable Compressed ROM `Support" OFF) ## Probably should be a default option. 
	option(DAEDALUS_SILENT "Shhh, no debug console message, on by default" ON)

	# Additional Debug Options
	option(DAEDALUS_DEBUG_MEMORY "Debug memory issues" OFF)
	option(DAEDALUS_DEBUG_PIF "Enable Debugging Options for Peripheral Interface (PIF)" OFF)
	option(DAEDALUS_DEBUG_DYNAREC "Enable Various Debugging options for the Dynarec" OFF)
	option(DAEDALUS_ENABLE_SYNCHRONISATION "Enable for sync testing" OFF)
	option(DAEDALUS_ENABLE_PROFILING "Enable built in profiler" OFF)
	option(DAEDALUS_PROFILE_EXECUTION "Keep track of various execution stats" OFF)
	option(DUMPOSFUNCTIONS "Dump OSHLE Functions" OFF)
	# Additional PSP Options
	option(DAEDALUS_PSP_GPROF "PSP Profiling, needs Silent enabled" OFF)
	option(DAEDALUS_SDL "SDL PSP Stuff, Still in testing" OFF)

	## Enable specific options per platform based on Release build ## XXX need to handle GPROF
	# Handle specific options (These were all found in platforms.h and now belong here)
	# PSP / Vita / CTR options are required as non-linux embedded platforms are not able to be reliably detected.
	# Windows, Linux and macOS builds are auto detected.

	if(PSP) 
		option(DAEDALUS_DIALOGS "Enable UI Dialogs" ON)
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynamic Recompilation, Disable if system not supported" ON)
		option(DAEDALUS_PSP "PSP specific Options" ON)
		option(DAEDALUS_PSP_USE_ME "Use the Media Engine" ON)
		option(DAEDALUS_PSP_USE_VFPU "Enable VFPU Instructions" ON)
		option(DAEDALUS_SIM_DOUBLES "Simulate Doubles" ON)
	elseif(UNIX)
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynamic Recompilation, Disable if system not supported" OFF)
		option(DAEDALUS_POSIX "Posix specific options" ON)
		option(DAEDALUS_ACCURATE_TMEM "Accurate TMEM" ON)
		option(DAEDALUS_128BIT_MULT "128 Bit Multiplication, not supported on PSP" ON)
		option(DAEDALUS_DAEDALUS_ACCURATE_CVT "Convert using the rounding mode specified in the Floating Control/Status register (FCSR)")
	elseif(WIN32)
		option(DAEDALUS_WIN32 "Windows specific Options" ON)
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynamic Recompilation, Disable if system not supported" ON)
		option(DAEDALUS_ACCURATE_TMEM "Accurate TMEM" ON)
		option(DAEDALUS_BREAKPOINTS_ENABLED "Enable Breakpoints" OFF)
	elseif(CTR)
		option(DAEDALUS_CTR "3DS Specific Options" ON)
		option(DAEDALUS_ENABLE_DYNAREC "Enable Dynamic Recompilation" ON)
		option(DAEDALUS_ENABLE_OS_HOOKS " High Level Emulation Hooks" ON)

	endif()


	# Needed below the options to post process
	include(${PROJECT_SOURCE_DIR}/Options.cmake) 
	

	## Generic defines for certain things that Windows requires.
	if(NOT WIN32)
		add_compile_definitions(R4300_CALL_TYPE=${})
		add_compile_definitions(DAEDALUS_THREAD_CALL_TYPE=${})
		add_compile_definitions(DAEDALUS_VARARG_CALL_TYPE=${})
		add_compile_definitions(DAEDALUS_ZLIB_CALL_TYPE=${})
	endif()

	# Include the base directory first as BuildOptions.h contains some function Macros
	# As of writing CMake does not yet support function Macros

	include_directories(BEFORE .)

	# Linker and executable targets are here
	if (PSP)
		set(PLAT_EXTRA Graphics_Plat Input_Plat UI_Plat Plugins_Plat UI_Plat )
		set(PLAT_LIBS g psppower pspgu pspctrl pspdebug pspsdk pspdisplay pspge pspaudiolib pspaudio intrafont pspkubridge pspfpu pspge ${SDL_LIBS})
	
		## SDL Libraries for use with the experimental SDL Audio Plugin
		if (DAEDALUS_SDL)
			set(SDL_LIBS SDL2 SDL2main pspvram psphprm GL pspvfpu )
		endif (DAEDALUS_SDL)
		
		add_executable(daedalus SysPSP/main.cpp)

	#Generate Eboot, This might be better to do in the .sh file given the complexity it adds
		create_pbp_file(
		TARGET daedalus
		ICON_PATH ${PROJECT_SOURCE_DIR}/Source/SysPSP/Resources/eboot_icons/icon0.png
		BACKGROUND_PATH ${PROJECT_SOURCE_DIR}/Source/SysPSP/Resources/eboot_icons/pic1.png
		PREVIEW_PATH NULL
		TITLE DaedalusX64
		BUILD_PRX
					)
 	endif (PSP)

	if (APPLE)
		set (APPLE_PLAT_LIBS "-framework OpenGL -framework AudioToolbox -framework Cocoa -framework IOKit -framework CoreVideo")
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
		find_package(SDL2 CONFIG REQUIRED)
		set(PLAT_EXTRA Graphics_GL HLEGraphics_GL Input_GL Interface_GL imgui)
		if (UNIX AND NOT APPLE)
		set(GL_LIB GL)
		endif()
		set(PLAT_LIBS png z minizip pthread GLEW SDL2 dl )
		add_definitions( -DDAEDALUS_GL)
		 include_directories(BEFORE ${PROJECT_SOURCE_DIR}/Source/third_party/imgui ${SDL2_INCLUDE_DIRS})
		add_executable(daedalus SysPosix/main.cpp)
	endif (UNIX)

if (CTR)
	set (PLAT_EXTRA Graphics_Plat Input_Plat UI_Plat )
	set (PLAT_LIBS) ## TODO
endif(CTR)

if (VITA) 
	set (PLAT_EXTRA)
	set (PLAT_LIBS)
endif(VITA)

if(WIN32)
	set(CMAKE_TOOLCHAIN_FILE ${VCPKG_TOOLCHAIN})
	find_package(ZLIB)
	find_package(SDL2 CONFIG REQUIRED)
	find_package(GLEW REQUIRED)
		
	add_definitions(-DDAEDALUS_GL)
	set(PLAT_LIBS png z minizip pthread GLEW SDL2 dl ${OS_LIBS})
	# Pull in modules for glew / SDL2 / ZLib / LibPNG / Minizip
	include_directories(BEFORE .)

#	add_library(sysGL OBJECT ${SYSGL_BUILD})
#	target_link_libraries(sysGL OpenGL32 glfw3 glew32s SDL2 Xinput ${OS_LIBS})

#	add_library(daedalus.lib OBJECT ${BUILD} ${WIN_BUILD} ${AUDIO_DRIVER} )
#	target_link_libraries(daedalus.lib dsound dxguid sysGL libpng zlib zlibwapi)

	#Build and Link Executable
	add_executable(daedalus SysW32/main.cpp)
	target_link_libraries(daedalus LINK_PUBLIC	
										   Core Config Debug  Graphics HLEAudio HLEGraphics Interface Math OSHLE RomFile System Utility 
										   Graphics_GL HLEGraphics_GL Input_GL Interface_GL imgui
										   Win_Debug Win_Audio Win_Utility X86_Dynarec)
	endif (WIN32)

		# Build command
		set(CORE Core Config Debug ${DYNAREC} Graphics HLEAudio HLEGraphics Interface Math OSHLE RomFile System Utility)
		set(PLAT_CORE Debug_Plat DynaRec_Plat HLEAudio_Plat HLEGraphics_Plat Utility_Plat)
		set(DEFAULT_LIBS stdc++ png z c m pthread )
		target_link_libraries(daedalus LINK_PUBLIC ${CORE} ${PLAT_CORE} ${PLAT_EXTRA} ${APPLE_PLAT_LIBS} ${GL_LIB} ${DEFAULT_LIBS} ${PLAT_LIBS}  )
	

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
	add_subdirectory(third_party/imgui)
	elseif(WIN32) ## Probably keep this generic as we'll build this on Linux too?
	add_subdirectory(SysGL)
	add_subdirectory(SysW32)
	elseif(CTR)
	add_subdirectory(SysCTR)
	else(PSP)
	add_subdirectory(SysPSP)

	endif()

	## Install and setup basic directories
	install(DIRECTORY DESTINATION DaedalusX64)
	install(DIRECTORY DESTINATION DaedalusX64/Roms)
	install(DIRECTORY DESTINATION DaedalusX64/SaveGames)
	install(DIRECTORY DESTINATION DaedalusX64/SaveStates)