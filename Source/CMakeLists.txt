#CMake File for DaedalusX64

#Build instructions
# Create a directory in the DaedalusX64 root directory and cd into it 'eg: mkdir tmp ~/DaedalusX64'
# ensure you have cmake installed
# For PSP:
# cmake -DCMAKE_TOOLCHAIN_FILE=../Tools/psptoolchain.cmake ../Source/ -DPSP_OPTION

#Options
# PSP_RELEASE - Builds PSP Release

cmake_minimum_required(VERSION 3.7)

project (DaedalusX64)




#Base Source files


set (BASE_FILES StdAfx.cpp)
set (CONFIG_FILES Config/ConfigOptions.cpp)
set (CORE_FILES Core/Cheats.cpp Core/CPU.cpp Core/DMA.cpp Core/Dynamo.cpp Core/FlashMem.cpp Core/Interpret.cpp Core/Interrupts.cpp Core/JpegTask.cpp Core/Memory.cpp Core/PIF.cpp Core/R4300.cpp Core/Registers.cpp Core/ROM.cpp Core/ROMBuffer.cpp Core/ROMImage.cpp Core/RomSettings.cpp Core/RSP_HLE.cpp Core/Save.cpp Core/SaveState.cpp Core/TLB.cpp)
set (DEBUG_FILES Debug/DebugConsoleImpl.cpp Debug/DebugLog.cpp Debug/Dump.cpp)
set (DYNAREC_FILES DynaRec/BranchType.cpp DynaRec/DynaRecProfile.cpp DynaRec/Fragment.cpp DynaRec/FragmentCache.cpp DynaRec/IndirectExitMap.cpp DynaRec/StaticAnalysis.cpp DynaRec/TraceRecorder.cpp)
set (GRAPHICS_FILES Graphics/ColourValue.cpp Graphics/PngUtil.cpp Graphics/TextureTransform.cpp)
set (HLEAUDIO_FILES HLEAudio/ABI1.cpp HLEAudio/ABI2.cpp HLEAudio/ABI3.cpp HLEAudio/ABI3mp3.cpp HLEAudio/AudioBuffer.cpp HLEAudio/AudioHLEProcessor.cpp HLEAudio/HLEMain.cpp)
set (HLEGRAPHICS_FILES HLEGraphics/BaseRenderer.cpp HLEGraphics/CachedTexture.cpp HLEGraphics/ConvertImage.cpp HLEGraphics/ConvertTile.cpp HLEGraphics/DLDebug.cpp HLEGraphics/DLParser.cpp HLEGraphics/Microcode.cpp HLEGraphics/RDP.cpp  HLEGraphics/RDPStateManager.cpp  HLEGraphics/TextureCache.cpp HLEGraphics/TextureInfo.cpp HLEGraphics/uCodes/Ucode.cpp)
set (INTERFACE_FILES Interface/RomDB.cpp)
set (MATH_FILES Math/Matrix4x4.cpp)
set (OSHLE_FILES OSHLE/OS.cpp OSHLE/patch.cpp)
set (PLUGIN_FILES Plugins/GraphicsPlugin.cpp)
set (SYSTEM_FILES System/Paths.cpp System/System.cpp)
set (TEST_FILES Test/BatchTest.cpp)
set (UTILITY_FILES Utility/CRC.cpp Utility/DataSink.cpp Utility/FastMemcpy.cpp  Utility/FramerateLimiter.cpp Utility/Hash.cpp Utility/IniFile.cpp Utility/MemoryHeap.cpp Utility/Preferences.cpp Utility/PrintOpCode.cpp Utility/Profiler.cpp Utility/ROMFile.cpp Utility/ROMFileCache.cpp Utility/ROMFileCompressed.cpp Utility/ROMFileMemory.cpp Utility/ROMFileUncompressed.cpp Utility/Stream.cpp Utility/StringUtil.cpp Utility/Synchroniser.cpp Utility/Timer.cpp Utility/Translate.cpp Utility/ZLibWrapper.cpp)
set (UNKNOWN_FILES Utility/FastMemcpy_test.cpp Utility/MemoryPool.cpp)

set (BUILD ${BASE_FILES} ${CONFIG_FILES} ${CORE_FILES} ${DEBUG_FILES} ${DYNAREC_FILES} ${GRAPHICS_FILES} ${HLEAUDIO_FILES} ${HLEGRAPHICS_FILES} ${INTERFACE_FILES} ${MATH_FILES} ${OSHLE_FILES} ${PLUGIN_FILES} ${SYSTEM_FILES} ${TEST_FILES} ${UTILITY_FILES})

#PSP Source files
set (PSP_DEBUG_FILES SysPSP/Debug/DaedalusAssertPSP.cpp SysPSP/Debug/DBGConsolePSP.cpp)
set (PSP_DVEMGR_FILES SysPSP/DveMgr/pspDveManager.S)
set (PSP_DYNAREC_FILES SysPSP/DynaRec/AssemblyWriterPSP.cpp SysPSP/DynaRec/AssemblyUtilsPSP.cpp SysPSP/DynaRec/CodeBufferManagerPSP.cpp SysPSP/DynaRec/CodeGeneratorPSP.cpp SysPSP/DynaRec/DynaRecStubs.S SysPSP/DynaRec/DynarecTargetPSP.cpp SysPSP/DynaRec/N64RegisterCachePSP.cpp)
set (PSP_GRAPHICS_FILES SysPSP/Graphics/DrawText.cpp SysPSP/Graphics/GraphicsContext.cpp SysPSP/Graphics/NativeTexturePSP.cpp SysPSP/Graphics/VideoMemoryManager.cpp SysPSP/Graphics/intraFont/intraFont.c SysPSP/Graphics/intraFont/libccc.c)
set (PSP_HLEAUDIO_FILES SysPSP/HLEAudio/AudioOutput.cpp SysPSP/HLEAudio/AudioPluginPSP.cpp)
set (PSP_HLEGRAPHICS_FILES SysPSP/HLEGraphics/Blender.cpp SysPSP/HLEGraphics/BlendModes.cpp SysPSP/HLEGraphics/ColourAdjuster.cpp SysPSP/HLEGraphics/ConvertVertices.S SysPSP/HLEGraphics/DisplayListDebugger.cpp SysPSP/HLEGraphics/GraphicsPluginPSP.cpp SysPSP/HLEGraphics/RendererPSP.cpp SysPSP/HLEGraphics/Combiner/BlendConstant.cpp SysPSP/HLEGraphics/Combiner/CombinerExpression.cpp SysPSP/HLEGraphics/Combiner/CombinerTree.cpp SysPSP/HLEGraphics/Combiner/RenderSettings.cpp SysPSP/HLEGraphics/TnLVFPU.S SysPSP/HLEGraphics/VectorClipping.S)
set (PSP_INPUTMANAGER_FILES SysPSP/Input/InputManagerPSP.cpp)
set (PSP_KERNELBUTTON_FILES SysPSP/KernelButtonsPrx/imposectrl.S)
set (PSP_MEDIAENGINEFILES SysPSP/MediaEnginePRX/me.c SysPSP/MediaEnginePRX/MediaEngine.S )
set (PSP_UI_FILES SysPSP/UI/AboutComponent.cpp SysPSP/UI/AdjustDeadzoneScreen.cpp SysPSP/UI/AdvancedOptionsScreen.cpp SysPSP/UI/CheatOptionsScreen.cpp SysPSP/UI/ColourPulser.cpp SysPSP/UI/Dialogs.cpp SysPSP/UI/GlobalSettingsComponent.cpp SysPSP/UI/MainMenuScreen.cpp SysPSP/UI/PauseOptionsComponent.cpp SysPSP/UI/PauseScreen.cpp SysPSP/UI/RomPreferencesScreen.cpp SysPSP/UI/RomSelectorComponent.cpp SysPSP/UI/SavestateSelectorComponent.cpp SysPSP/UI/SelectedRomComponent.cpp SysPSP/UI/SplashScreen.cpp SysPSP/UI/UICommand.cpp SysPSP/UI/UIComponent.cpp SysPSP/UI/UIContext.cpp SysPSP/UI/UIElement.cpp SysPSP/UI/UIScreen.cpp SysPSP/UI/UISetting.cpp)
set (PSP_UTILITY_FILES SysPSP/Utility/AtomicPrimitives.S SysPSP/Utility/BatteryPSP.cpp SysPSP/Utility/Buttons.cpp SysPSP/Utility/DebugMemory.cpp SysPSP/Utility/DisableFPUExceptions.S SysPSP/Utility/exception.cpp SysPSP/Utility/FastMemcpyPSP.cpp SysPSP/Utility/IOPSP.cpp SysPSP/Utility/JobManager.cpp SysPSP/Utility/ModulePSP.cpp SysPSP/Utility/ThreadPSP.cpp SysPSP/Utility/TimingPSP.cpp SysPSP/Utility/PSPModel.cpp SysPSP/Utility/VolatileMemPSP.cpp)
set (GPROF_SRCS SysPSP/Debug/prof.c SysPSP/Debug/mcount.S )
set (PSP_MAIN_FILES SysPSP/main.cpp)
set (PSP_BUILD ${PSP_DEBUG_FILES} ${PSP_DVEMGR_FILES} ${PSP_DYNAREC_FILES} ${PSP_GRAPHICS_FILES} ${PSP_HLEAUDIO_FILES} ${PSP_HLEGRAPHICS_FILES} ${PSP_INPUTMANAGER_FILES} ${PSP_KERNELBUTTON_FILES} ${PSP_MEDIAENGINEFILES} ${PSP_UI_FILES} ${PSP_UTILITY_FILES})


#Handle .S files - Probably a better way to do this
set_property(SOURCE SysPSP/HLEGraphics/ConvertVertices.S PROPERTY LANGUAGE C)
set_property(SOURCE SysPSP/DynaRec/DynaRecStubs.S PROPERTY LANGUAGE C)
set_property(SOURCE SysPSP/HLEGraphics/TnLVFPU.S PROPERTY LANGUAGE C)
set_property(SOURCE SysPSP/HLEGraphics/VectorClipping.S PROPERTY LANGUAGE C)
set_property(SOURCE SysPSP/Utility/AtomicPrimitives.S PROPERTY LANGUAGE C)
set_property(SOURCE SysPSP/Utility/DisableFPUExceptions.S PROPERTY LANGUAGE C)
set_property(SOURCE SysPSP/Debug/mcount.S PROPERTY LANGUAGE C)
set_property(SOURCE SysPSP/DveMgr/pspDveManager.S PROPERTY LANGUAGE C)
set_property(SOURCE SysPSP/KernelButtonsPrx/imposectrl.S PROPERTY LANGUAGE C)
set_property(SOURCE SysPSP/MediaEnginePRX/MediaEngine.S PROPERTY LANGUAGE C)


#Windows BUILD
set (WIN_AUDIO SysW32/HLEAudio/AudioPluginW32.cpp)
set (WIN_DEBUG SysW32/Debug/DaedalusAssertW32.cpp SysW32/Debug/DebugConsoleW32.cpp)
set (WIN_UTILITY SysW32/Utility/IOW32.cpp SysW32/Utility/ThreadW32.cpp SysW32/Utility/TimingW32.cpp)

set (WIN_BUILD ${WIN_AUDIO} ${WIN_DEBUG} ${WIN_UTILITY})

#macOS Build
set (MAC_DEBUG SysOSX/Debug/DaedalusAssertOSX.cpp SysOSX/Debug/DebugConsoleOSX.cpp SysOSX/Debug/WebDebug.cpp SysOSX/Debug/WebDebugTemplate.cpp)
set (MAC_DYNAREC SysOSX/DynaRec/CodeBufferManagerOSX.cpp)
set (MAC_HLEGRAPHICS SysOSX/HLEGraphics/DisplayListDebugger.cpp)
set (MAC_MAIN_FILES SysOSX/main.cpp)
set (MAC_BUILD ${MAC_DEBUG} ${MAC_DYNAREC} ${MAC_HLEGRAPHICS} ${POSIX_UTILITY})

#Posix BUILD

set (POSIX_UTILITY SysPosix/Utility/CondPosix.cpp SysPosix/Utility/IOPosix.cpp SysPosix/Utility/ThreadPosix.cpp SysPosix/Utility/TimingPosix.cpp)

set (LINUX_AUDIO SysLinux/HLEAudio/AudioPluginLinux.cpp)

set (LINUX_BUILD ${MAC_BUILD} ${LINUX_AUDIO})

#SysGL
set (SYSGL_GRAPHICS SysGL/Graphics/GraphicsContextGL.cpp SysGL/Graphics/NativeTextureGL.cpp)
set (SYSGL_HLEGRAPHICS SysGL/HLEGraphics/GraphicsPluginGL.cpp SysGL/HLEGraphics/RendererGL.cpp)
set (SYSGL_INPUT SysGL/Input/InputManagerGL.cpp)
set (SYSGL_INTERFACE SysGL/Interface/UI.cpp)
set (SYSGL_BUILD ${SYSGL_GRAPHICS} ${SYSGL_HLEGRAPHICS} ${SYSGL_INPUT} ${SYSGL_INTERFACE})






Set(PSPSDK_LIBS -lpspdebug -lpspdisplay -lpspge -lpspctrl -lpspsdk -lpsplibc -lpsputility -lpspuser -lpspkernel )


if (PSP_RELEASE)
	message("PSP Release Build..")
	add_definitions("-DNDEBUG -O3")
	add_compile_options(-DPSP_LARGE_MEMORY=1 -DPSP_FW_VERSION=661)
	add_definitions(-DDAEDALUS_PSP)

	include_directories(${PROJECT_SOURCE_DIR}/Config/Release)
	include_directories(BEFORE ${PROJECT_SOURCE_DIR}/SysPSP/Include)
	add_library(daedalus STATIC ${BUILD} ${PSP_BUILD})
	add_executable(daedalus.elf ${PSP_MAIN_FILES})
	set_target_properties(daedalus.elf PROPERTIES LINK_FLAGS "-specs=${PSPSDK}/lib/prxspecs -Wl,-q,-T${PSPSDK}/lib/linkfile.prx ${PSPSDK}/lib/prxexports.o -L${PSPSDK}/lib")
	target_include_directories(daedalus.elf PUBLIC /usr/local/pspdev/psp/sdk/include )
	target_link_libraries(daedalus.elf daedalus -lstdc++ -lpsppower -lpspgu -lpspaudio -lpsprtc -lpng -lz -lg -lm -lpspfpu ${PSPSDK_LIBS})
endif (PSP_RELEASE)



if (PSP_DEBUG)
	message("PSP Debug Build..")
	include_directories(${PROJECT_SOURCE_DIR}/Config/Dev)
	add_definitions("-g -O1 -fno-omit-frame-pointer -G0 -D_DEBUG -MD -Wall")
	add_compile_options(-DPSP_LARGE_MEMORY=1 -DPSP_FW_VERSION=661)
	add_definitions(-DDAEDALUS_PSP )
	include_directories(BEFORE ${PROJECT_SOURCE_DIR}/SysPSP/Include)
	add_library(daedalus STATIC ${BUILD} ${PSP_BUILD})
	add_executable(daedalus.elf ${PSP_MAIN_FILES})
	set_target_properties(daedalus.elf PROPERTIES LINK_FLAGS "-specs=${PSPSDK}/lib/prxspecs -Wl,-q,-T${PSPSDK}/lib/linkfile.prx ${PSPSDK}/lib/prxexports.o -L${PSPSDK}/lib")
	target_include_directories(daedalus.elf PUBLIC /usr/local/pspdev/psp/sdk/include )
	target_link_libraries(daedalus.elf daedalus -lstdc++ -lpsppower -lpspgu -lpspaudio -lpsprtc -lpng -lz -lg -lm -lpspfpu  ${PSPSDK_LIBS})
endif (PSP_DEBUG)

if (LINUX_RELEASE)
	message("Linux Release Build..")
	add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/zlib)
	include_directories(${PROJECT_SOURCE_DIR}/Config/Release ${PROJECT_SOURCE_DIR}/Debug ${PROJECT_SOURCE_DIR}/third_party/zlib ${PROJECT_SOURCE_DIR}/third_party/zlib/contrib/minizip)
	add_definitions("-DNDEBUG -O3 -pedantic")
		include_directories(BEFORE /usr/local/include)
		add_library(glLib STATIC ${SYSGL_BUILD} )
				target_include_directories(glLib PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/Linux/Include )
		add_library(daedalus.lib STATIC ${BUILD} ${LINUX_BUILD})
					target_include_directories(daedalus.lib PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/SysLinux/Include )
			add_executable(daedalus ${MAC_MAIN_FILES})
		set_target_properties(daedalus PROPERTIES LINK_FLAGS -L/usr/local/lib)
					target_include_directories(daedalus PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/SysOSX/Include )
										link_directories(BEFORE /usr/local/lib)
										target_link_libraries(daedalus daedalus.lib glLib -lglew -lpng z minizip)
endif (LINUX_RELEASE)


if (MAC_RELEASE)
	message("macOS Release Build..")
	add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/zlib)
	include_directories(${PROJECT_SOURCE_DIR}/Config/Release ${PROJECT_SOURCE_DIR}/Debug ${PROJECT_SOURCE_DIR}/third_party/zlib ${PROJECT_SOURCE_DIR}/third_party/zlib/contrib/minizip)
	add_definitions("-DNDEBUG -O3")
		include_directories(BEFORE /usr/local/include )
		add_library(glLib STATIC ${SYSGL_BUILD} )
				target_include_directories(glLib PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/SysOSX/Include )
		add_library(daedalus.lib STATIC ${BUILD} ${MAC_BUILD})
					target_include_directories(daedalus.lib PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/SysOSX/Include )
			add_executable(daedalus ${MAC_MAIN_FILES})
		set_target_properties(daedalus PROPERTIES LINK_FLAGS -L/usr/local/lib)
					target_include_directories(daedalus PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/SysOSX/Include )
										link_directories(BEFORE /usr/local/lib)
										target_link_libraries(daedalus daedalus.lib glLib -lglew "-framework Cocoa -framework IOKit -framework CoreVideo -framework AudioToolBox"  -lpng z minizip)
#			target_link_libraries (daedalus glLib daedalus.lib ${IOKit} ${OPENGL_glu_LIBRARY} ${COCOA_LIBRARY} ${AUDIO_TOOLBOX} ${ZIP_LIBRARY}  ${COREVIDEO} ${OPENGL} -lglfw3 -lglew  -lpng zip)
endif (MAC_RELEASE)
