project (DaedalusX64 LANGUAGES C CXX ASM )
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) 

	#Build Type
	if(DEBUG)
		set(CMAKE_BUILD_TYPE Debug)
		add_compile_definitions(DAEDALUS_CONFIG_VERSION="Dev")
		option(DAEDALUS_ENABLE_ASSERTS "Enable Static Asserts" ON)
		option(DAEDALUS_DEBUG_CONSOLE "Enable Debug Console" ON)
		option(DAEDALUS_DEBUG_DISPLAYLIST "Options for debugging the display list, useful for determining texture issues" ON)
		option(DAEDALUS_LOG "Enable various logging" ON)
		add_definitions("-g -D_DEBUG -MD ")
	else()
		set(CMAKE_BUILD_TYPE Release)
		add_compile_definitions(DAEDALUS_CONFIG_VERSION="Release")
	#	option(DAEDALUS_SILENT "Shhh, no debug console message, on by default" ON)
		add_compile_definitions(O3)
		add_compile_definitions(DNDEBUG)
		add_compile_definitions(fltfo)
	endif()

	# General options

	option(DAEDALUS_BATCH_TEST_ENABLED "Enable batch testing for compatibility" OFF)
	option(DAEDALUS_ENABLE_OS_HOOKS "Enable High Level Emulation Hooks" ON)
	option(DAEDALUS_COMPRESSED_ROM_SUPPORT "Enable Compressed ROM Support" OFF)
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

	# Core Build Files
	set (CONFIG_FILES Config/ConfigOptions.cpp)
	set (CORE_FILES Core/HvqmTask.cpp Core/RDRam.cpp Core/Cheats.cpp Core/CPU.cpp Core/DMA.cpp Core/Dynamo.cpp Core/FlashMem.cpp  Core/FramerateLimiter.cpp Core/Interpret.cpp Core/Interrupt.cpp Core/JpegTask.cpp Core/Memory.cpp Core/PIF.cpp Core/PrintOpCode.cpp Core/R4300.cpp Core/Registers.cpp Core/ROM.cpp Core/ROMBuffer.cpp Core/ROMImage.cpp Core/RomSettings.cpp Core/RSP_HLE.cpp Core/Save.cpp Core/TLB.cpp)
	set (DEBUG_FILES Debug/DebugConsoleImpl.cpp Debug/DebugLog.cpp Debug/Dump.cpp Debug/Synchroniser.cpp)
	set (DYNAREC_FILES DynaRec/BranchType.cpp DynaRec/DynaRecProfile.cpp DynaRec/Fragment.cpp DynaRec/FragmentCache.cpp DynaRec/IndirectExitMap.cpp DynaRec/StaticAnalysis.cpp DynaRec/TraceRecorder.cpp)
	set (GRAPHICS_FILES Graphics/ColourValue.cpp Graphics/PngUtil.cpp Graphics/TextureTransform.cpp)
	set (HLEAUDIO_FILES HLEAudio/HLEAudioState.cpp HLEAudio/ABI1.cpp HLEAudio/ABI2.cpp HLEAudio/ABI3.cpp HLEAudio/ABI3mp3.cpp HLEAudio/AudioBuffer.cpp HLEAudio/HLEMain.cpp HLEAudio/ABI_ADPCM.cpp HLEAudio/ABI_Buffers.cpp HLEAudio/ABI_Filters.cpp HLEAudio/ABI_MixerInterleave.cpp HLEAudio/ENV_Mixer.cpp HLEAudio/ABI_Resample.cpp)
	set (HLEGRAPHICS_FILES HLEGraphics/BaseRenderer.cpp HLEGraphics/BaseRenderer.h HLEGraphics/CachedTexture.cpp HLEGraphics/ConvertImage.cpp HLEGraphics/ConvertTile.cpp HLEGraphics/DLDebug.cpp HLEGraphics/DLParser.cpp HLEGraphics/Microcode.cpp HLEGraphics/RDP.cpp HLEGraphics/RDPStateManager.cpp HLEGraphics/TextureCache.cpp HLEGraphics/TextureCacheWebDebug.cpp HLEGraphics/TextureInfo.cpp HLEGraphics/TMEM.cpp HLEGraphics/uCodes/Ucode.cpp)
	set (INTERFACE_FILES Interface/RomDB.cpp Interface/Preferences.cpp Interface/SaveState.cpp )
	set (MATH_FILES Math/Matrix4x4.cpp)
	set (OSHLE_FILES OSHLE/OS.cpp OSHLE/patch.cpp)
	set (ROMFILE_FILES RomFile/RomFile.cpp RomFile/RomFileCache.cpp RomFile/RomFileCompressed.cpp RomFile/RomFileMemory.cpp RomFile/RomFileUncompressed.cpp)
	set (SYSTEM_FILES System/CompressedStream.cpp System/CompressedStream.cpp System/DataSink.cpp System/SystemInit.cpp)
	set (TEST_FILES Test/BatchTest.cpp)
	set (UTILITY_FILES Utility/CRC.cpp Utility/FastMemcpy.cpp Utility/Hash.cpp Utility/IniFile.cpp Utility/MemoryHeap.cpp  Utility/Profiler.cpp  Utility/Stream.cpp Utility/StringUtil.cpp Utility/Timer.cpp )
	set (UNKNOWN_FILES Utility/FastMemcpy_test.cpp Utility/MemoryPool.cpp)
	set (DEBUG_ONLY Core/Registers.cpp)
	set (BUILD ${BASE_FILES} ${CONFIG_FILES} ${CORE_FILES} ${DEBUG_FILES} ${DYNAREC_FILES} ${GRAPHICS_FILES} ${HLEAUDIO_FILES} ${HLEGRAPHICS_FILES} ${INTERFACE_FILES} ${MATH_FILES} ${OSHLE_FILES} ${PLUGIN_FILES} ${ROMFILE_FILES} ${SYSTEM_FILES} ${TEST_FILES} ${UTILITY_FILES})


	#PSP Build
	set (PSP_DEBUG_FILES SysPSP/Debug/DaedalusAssertPSP.cpp SysPSP/Debug/DBGConsolePSP.cpp SysPSP/DynaRec/DynaRecDebug.cpp )
	set (PSP_DVEMGR_FILES SysPSP/PRX/DveMgr/pspDveManager.S)
	set (PSP_DYNAREC_FILES SysPSP/DynaRec/AssemblyWriterPSP.cpp SysPSP/DynaRec/AssemblyUtilsPSP.cpp SysPSP/DynaRec/CodeBufferManagerPSP.cpp SysPSP/DynaRec/CodeGeneratorPSP.cpp SysPSP/DynaRec/DynaRecStubs.S SysPSP/DynaRec/N64RegisterCachePSP.cpp)
	set (PSP_GRAPHICS_FILES SysPSP/Graphics/DrawText.cpp SysPSP/Graphics/GraphicsContext.cpp SysPSP/Graphics/NativeTexturePSP.cpp SysPSP/Graphics/VideoMemoryManager.cpp)
	set (PSP_HLEAUDIO_FILES SysPSP/HLEAudio/AudioPluginPSP.cpp)
	set (PSP_HLEGRAPHICS_FILES SysPSP/HLEGraphics/Blender.cpp SysPSP/HLEGraphics/BlendModes.cpp SysPSP/HLEGraphics/ColourAdjuster.cpp SysPSP/HLEGraphics/ConvertVertices.S SysPSP/HLEGraphics/DisplayListDebugger.cpp SysPSP/HLEGraphics/GraphicsPluginPSP.cpp SysPSP/HLEGraphics/RendererPSP.cpp SysPSP/HLEGraphics/Combiner/BlendConstant.cpp SysPSP/HLEGraphics/Combiner/CombinerExpression.cpp SysPSP/HLEGraphics/Combiner/CombinerTree.cpp SysPSP/HLEGraphics/Combiner/RenderSettings.cpp SysPSP/HLEGraphics/TnLVFPU.S SysPSP/HLEGraphics/VectorClipping.S)
	set (PSP_INPUTMANAGER_FILES SysPSP/Input/InputManagerPSP.cpp)
	set (PSP_KERNELBUTTON_FILES SysPSP/PRX/KernelButtons/imposectrl.S)
    set (PSP_MEDIAENGINEFILES SysPSP/PRX/MediaEngine/me.c SysPSP/PRX/MediaEngine/MediaEngine.S )
	set (PSP_UI_FILES SysPSP/UI/AboutComponent.cpp SysPSP/UI/AdjustDeadzoneScreen.cpp SysPSP/UI/AdvancedOptionsScreen.cpp SysPSP/UI/CheatOptionsScreen.cpp SysPSP/UI/ColourPulser.cpp SysPSP/UI/Dialogs.cpp SysPSP/UI/GlobalSettingsComponent.cpp SysPSP/UI/MainMenuScreen.cpp SysPSP/UI/PauseOptionsComponent.cpp SysPSP/UI/PauseScreen.cpp SysPSP/UI/RomPreferencesScreen.cpp SysPSP/UI/RomSelectorComponent.cpp SysPSP/UI/SavestateSelectorComponent.cpp SysPSP/UI/SelectedRomComponent.cpp SysPSP/UI/SplashScreen.cpp SysPSP/UI/UICommand.cpp SysPSP/UI/UIComponent.cpp SysPSP/UI/UIContext.cpp SysPSP/UI/UIElement.cpp SysPSP/UI/UIScreen.cpp SysPSP/UI/UISetting.cpp)
	set (PSP_UTILITY_FILES SysPSP/Utility/AtomicPrimitives.S SysPSP/Utility/BatteryPSP.cpp SysPSP/Utility/Buttons.cpp SysPSP/Utility/DebugMemory.cpp SysPSP/Utility/DisableFPUExceptions.S SysPSP/Utility/exception.cpp SysPSP/Utility/FastMemcpyPSP.cpp SysPSP/Utility/JobManager.cpp SysPSP/Utility/ModulePSP.cpp SysPSP/Utility/TimingPSP.cpp SysPSP/Utility/Translate.cpp SysPSP/Utility/VolatileMemPSP.cpp)
	set (GPROF_SRCS SysPSP/Debug/prof.c SysPSP/Debug/mcount.S )
	set (PSP_POSIX SysPosix/Utility/ThreadPosix.cpp SysPosix/Utility/IOPosix.cpp)
	set (PSP_MAIN_FILES SysPSP/main.cpp)
	set (PSP_BUILD ${PSP_DEBUG_FILES} ${PSP_DVEMGR_FILES} ${PSP_DYNAREC_FILES} ${PSP_GRAPHICS_FILES} ${PSP_HLEAUDIO_FILES} ${PSP_HLEGRAPHICS_FILES} ${PSP_INPUTMANAGER_FILES} ${PSP_KERNELBUTTON_FILES} ${PSP_MEDIAENGINEFILES} ${PSP_POSIX} ${PSP_UI_FILES} ${PSP_UTILITY_FILES})
		
	#OpenGL
	set (SYSGL_GRAPHICS SysGL/Graphics/GraphicsContextGL.cpp SysGL/Graphics/NativeTextureGL.cpp)
	set (SYSGL_HLEGRAPHICS SysGL/HLEGraphics/GraphicsPluginGL.cpp SysGL/HLEGraphics/RendererGL.cpp)
	set (SYSGL_INPUT SysGL/Input/InputManagerGL.cpp)
	set (SYSGL_INTERFACE SysGL/Interface/UI.cpp)
	set (SYSGL_BUILD ${SYSGL_GRAPHICS} ${SYSGL_HLEGRAPHICS} ${SYSGL_INPUT} ${SYSGL_INTERFACE} ${PLUGIN_FILES})

	#Windows -- Not sure if it works yet
	set (WIN_AUDIO SysW32/HLEAudio/AudioPluginW32.cpp)
	set (WIN_DEBUG SysW32/Debug/DaedalusAssertW32.cpp)
	set (WIN_DYNAREC SysW32/DynaRec/x86/AssemblyUtilsX86.cpp SysW32/DynaRec/x86/AssemblyWriterX86.cpp SysW32/DynaRec/x86/CodeBufferManagerX86.cpp SysW32/DynaRec/x86/CodeGeneratorX86.cpp)
	set (WIN_INTERFACE SysW32/Interface/FileNameHandler.cpp)
	set (WIN_MAIN_FILES SysW32/main.cpp)
	set (WIN_UTILITY SysW32/Utility/IOW32.cpp SysW32/Utility/ThreadW32.cpp SysW32/Utility/TimingW32.cpp SysW32/Utility/CondW32.cpp SysW32/Utility/DISASM86.cpp)
	set (WIN_BUILD ${WIN_DYNAREC} ${WIN_AUDIO} ${WIN_DEBUG} ${WIN_INTERFACE} ${WIN_UTILITY})

	#Posix
	set (POSIX_DEBUG SysPosix/Debug/DaedalusAssertPosix.cpp SysPosix/Debug/DebugConsolePosix.cpp SysPosix/Debug/WebDebug.cpp SysPosix/Debug/WebDebugTemplate.cpp third_party/webby/webby.c)
	set (POSIX_DYNAREC SysPosix/DynaRec/CodeBufferManagerPosix.cpp)
	set (POSIX_HLEGRAPHICS SysPosix/HLEGraphics/DisplayListDebugger.cpp)
	set (POSIX_MAIN_FILES SysPosix/main.cpp)
	set (POSIX_UTILITY SysPosix/Utility/CondPosix.cpp SysPosix/Utility/IOPosix.cpp SysPosix/Utility/ThreadPosix.cpp SysPosix/Utility/TimingPosix.cpp)
	set (POSIX_BUILD ${POSIX_DEBUG} ${POSIX_DYNAREC} ${POSIX_HLEGRAPHICS} ${POSIX_UTILITY})

	# These will remain separate for now..
	set (LINUX_AUDIO SysPosix/HLEAudio/AudioPluginLinux.cpp)
	set (MAC_AUDIO SysPosix/HLEAudio/AudioPluginOSX.cpp)

	if (PSP)
		set (PLAT PSP)


		# add_compile_definitions(DAEDALUS_DIALOGS)

		include_directories(BEFORE ${PROJECT_SOURCE_DIR}/SysPSP/Include .)
		set(PLAT_LIBS psppower pspgu pspctrl pspdebug pspsdk pspdisplay pspge -lpspaudiolib -lpspaudio -lintrafont -lpspkubridge -lpspfpu)
 
 
	##Universal build options - Generic
 	add_compile_definitions(${PLAT}) 
	add_library(daedalus.lib STATIC ${BUILD} ${${PLAT}_BUILD})
	add_executable(daedalus ${${PLAT}_MAIN_FILES})

	target_link_libraries(daedalus daedalus.lib -lstdc++ -lpng -lz -lc -lg -lm -lpthread ${PLAT_LIBS} )
 
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
		include_directories(${PROJECT_SOURCE_DIR/HLEGraphics}) # Why are we including this?
		FIND_PATH(OPENGL_INCLUDE_DIR gl.h) #Is this required?
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		include_directories(/usr/local/include -framework OpenGL) # Is this required? 
		link_directories(/usr/local/lib) ## Is this required?
	endif (APPLE)

	if (UNIX)
		add_definitions(-DDAEDALUS_POSIX -DDAEDALUS_GL)
		include_directories(BEFORE ${PROJECT_SOURCE_DIR}/SysPosix/Include ${PROJECT_SOURCE_DIR})

		add_library(sysGL STATIC ${SYSGL_BUILD})
		target_link_libraries(sysGL GLEW SDL2 dl ${OS_LIBS})

		add_library(daedalus.lib STATIC ${BUILD} ${POSIX_BUILD} ${AUDIO_DRIVER} )
		target_link_libraries(daedalus.lib sysGL png z minizip pthread)

		add_executable(daedalus ${POSIX_MAIN_FILES})
		target_link_libraries(daedalus LINK_PUBLIC daedalus.lib )
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