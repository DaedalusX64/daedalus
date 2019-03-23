 #CMake File for DaedalusX64

#Build instructions
# Create a directory in the DaedalusX64 root directory and cd into it 'eg: mkdir tmp ~/DaedalusX64'
# ensure you have cmake installed
# For PSP:
# cmake -DCMAKE_TOOLCHAIN_FILE=../Tools/psptoolchain.cmake ../Source/ -DPSP_OPTION

#Options
# PSP_RELEASE - Builds PSP Release

cmake_minimum_required(VERSION 3.7)
set(CMAKE_CXX_STANDARD 14)
project (DaedalusX64)

				#Default Files for build
				set (BASE_FILES StdAfx.cpp)
				set (CONFIG_FILES Config/ConfigOptions.cpp)
				set (CORE_FILES Core/Cheats.cpp Core/CPU.cpp Core/DMA.cpp Core/Dynamo.cpp Core/FlashMem.cpp Core/Interpret.cpp Core/Interrupts.cpp Core/JpegTask.cpp Core/Memory.cpp Core/PIF.cpp Core/R4300.cpp Core/ROM.cpp Core/ROMBuffer.cpp Core/ROMImage.cpp Core/RomSettings.cpp Core/RSP_HLE.cpp Core/Save.cpp Core/SaveState.cpp Core/TLB.cpp)
				set (DEBUG_FILES Debug/DebugConsoleImpl.cpp Debug/DebugLog.cpp Debug/Dump.cpp)
				set (DYNAREC_FILES DynaRec/BranchType.cpp DynaRec/DynaRecProfile.cpp DynaRec/Fragment.cpp DynaRec/FragmentCache.cpp DynaRec/IndirectExitMap.cpp DynaRec/StaticAnalysis.cpp DynaRec/TraceRecorder.cpp)
				set (GRAPHICS_FILES Graphics/ColourValue.cpp Graphics/PngUtil.cpp Graphics/TextureTransform.cpp)
				set (HLEAUDIO_FILES HLEAudio/ABI1.cpp HLEAudio/ABI2.cpp HLEAudio/ABI3.cpp HLEAudio/ABI3mp3.cpp HLEAudio/AudioBuffer.cpp HLEAudio/AudioHLEProcessor.cpp HLEAudio/HLEMain.cpp)
				set (HLEGRAPHICS_FILES HLEGraphics/BaseRenderer.cpp HLEGraphics/BaseRenderer.h HLEGraphics/CachedTexture.cpp HLEGraphics/ConvertImage.cpp HLEGraphics/ConvertTile.cpp HLEGraphics/DLDebug.cpp HLEGraphics/DLParser.cpp HLEGraphics/Microcode.cpp HLEGraphics/RDP.cpp  HLEGraphics/RDPStateManager.cpp  HLEGraphics/TextureCache.cpp HLEGraphics/TextureInfo.cpp HLEGraphics/uCodes/Ucode.cpp)
				set (INTERFACE_FILES Interface/RomDB.cpp)
				set (MATH_FILES Math/Matrix4x4.cpp)
				set (OSHLE_FILES OSHLE/OS.cpp OSHLE/patch.cpp)
				set (PLUGIN_FILES Plugins/GraphicsPlugin.cpp)
				set (SYSTEM_FILES System/Paths.cpp System/System.cpp)
				set (TEST_FILES Test/BatchTest.cpp)
				set (UTILITY_FILES Utility/CRC.cpp Utility/DataSink.cpp Utility/FastMemcpy.cpp  Utility/FramerateLimiter.cpp Utility/Hash.cpp Utility/IniFile.cpp Utility/MemoryHeap.cpp Utility/Preferences.cpp Utility/PrintOpCode.cpp Utility/Profiler.cpp Utility/ROMFile.cpp Utility/ROMFileCache.cpp Utility/ROMFileCompressed.cpp Utility/ROMFileMemory.cpp Utility/ROMFileUncompressed.cpp Utility/Stream.cpp Utility/StringUtil.cpp Utility/Synchroniser.cpp Utility/Timer.cpp  Utility/Translate.cpp Utility/ZLibWrapper.cpp)
				set (UNKNOWN_FILES Utility/FastMemcpy_test.cpp Utility/MemoryPool.cpp)
        set (DEBUG_ONLY Core/Registers.cpp)
				set (BUILD ${BASE_FILES} ${CONFIG_FILES} ${CORE_FILES} ${DEBUG_FILES} ${DYNAREC_FILES} ${GRAPHICS_FILES} ${HLEAUDIO_FILES} ${HLEGRAPHICS_FILES} ${INTERFACE_FILES} ${MATH_FILES} ${OSHLE_FILES} ${PLUGIN_FILES} ${SYSTEM_FILES} ${TEST_FILES} ${UTILITY_FILES})


				#PSP Build
				set (PSP_DEBUG_FILES SysPSP/Debug/DaedalusAssertPSP.cpp SysPSP/Debug/DBGConsolePSP.cpp )
				set (PSP_DVEMGR_FILES SysPSP/PRX/DveMgr/pspDveManager.S)
				set (PSP_DYNAREC_FILES SysPSP/DynaRec/AssemblyWriterPSP.cpp SysPSP/DynaRec/AssemblyUtilsPSP.cpp SysPSP/DynaRec/CodeBufferManagerPSP.cpp SysPSP/DynaRec/CodeGeneratorPSP.cpp SysPSP/DynaRec/DynaRecStubs.S SysPSP/DynaRec/N64RegisterCachePSP.cpp)
				set (PSP_GRAPHICS_FILES SysPSP/Graphics/DrawText.cpp SysPSP/Graphics/GraphicsContext.cpp SysPSP/Graphics/NativeTexturePSP.cpp SysPSP/Graphics/VideoMemoryManager.cpp SysPSP/Graphics/intraFont/intraFont.c SysPSP/Graphics/intraFont/libccc.c)
				set (PSP_HLEAUDIO_FILES SysPSP/HLEAudio/AudioOutput.cpp SysPSP/HLEAudio/AudioPluginPSP.cpp)
				set (PSP_HLEGRAPHICS_FILES SysPSP/HLEGraphics/Blender.cpp SysPSP/HLEGraphics/BlendModes.cpp SysPSP/HLEGraphics/ColourAdjuster.cpp SysPSP/HLEGraphics/ConvertVertices.S SysPSP/HLEGraphics/DisplayListDebugger.cpp SysPSP/HLEGraphics/GraphicsPluginPSP.cpp SysPSP/HLEGraphics/RendererPSP.cpp SysPSP/HLEGraphics/Combiner/BlendConstant.cpp SysPSP/HLEGraphics/Combiner/CombinerExpression.cpp SysPSP/HLEGraphics/Combiner/CombinerTree.cpp SysPSP/HLEGraphics/Combiner/RenderSettings.cpp SysPSP/HLEGraphics/TnLVFPU.S SysPSP/HLEGraphics/VectorClipping.S)
				set (PSP_INPUTMANAGER_FILES SysPSP/Input/InputManagerPSP.cpp)
				set (PSP_KERNELBUTTON_FILES SysPSP/PRX/KernelButtons/imposectrl.S)
                set (PSP_MEDIAENGINEFILES SysPSP/PRX/MediaEngine/me.c SysPSP/PRX/MediaEngine/MediaEngine.S )
				set (PSP_UI_FILES SysPSP/UI/AboutComponent.cpp SysPSP/UI/AdjustDeadzoneScreen.cpp SysPSP/UI/AdvancedOptionsScreen.cpp SysPSP/UI/CheatOptionsScreen.cpp SysPSP/UI/ColourPulser.cpp SysPSP/UI/Dialogs.cpp SysPSP/UI/GlobalSettingsComponent.cpp SysPSP/UI/MainMenuScreen.cpp SysPSP/UI/PauseOptionsComponent.cpp SysPSP/UI/PauseScreen.cpp SysPSP/UI/RomPreferencesScreen.cpp SysPSP/UI/RomSelectorComponent.cpp SysPSP/UI/SavestateSelectorComponent.cpp SysPSP/UI/SelectedRomComponent.cpp SysPSP/UI/SplashScreen.cpp SysPSP/UI/UICommand.cpp SysPSP/UI/UIComponent.cpp SysPSP/UI/UIContext.cpp SysPSP/UI/UIElement.cpp SysPSP/UI/UIScreen.cpp SysPSP/UI/UISetting.cpp)
				set (PSP_UTILITY_FILES SysPSP/Utility/AtomicPrimitives.S SysPSP/Utility/BatteryPSP.cpp SysPSP/Utility/Buttons.cpp SysPSP/Utility/DebugMemory.cpp SysPSP/Utility/DisableFPUExceptions.S SysPSP/Utility/exception.cpp SysPSP/Utility/FastMemcpyPSP.cpp SysPSP/Utility/IOPSP.cpp SysPSP/Utility/JobManager.cpp SysPSP/Utility/ModulePSP.cpp SysPSP/Utility/ThreadPSP.cpp SysPSP/Utility/TimingPSP.cpp SysPSP/Utility/VolatileMemPSP.cpp)
				set (GPROF_SRCS SysPSP/Debug/prof.c SysPSP/Debug/mcount.S )
				set (PSP_MAIN_FILES SysPSP/main.cpp)

				set (PSP_BUILD ${PSP_DEBUG_FILES} ${PSP_DVEMGR_FILES} ${PSP_DYNAREC_FILES} ${PSP_GRAPHICS_FILES} ${PSP_HLEAUDIO_FILES} ${PSP_HLEGRAPHICS_FILES} ${PSP_INPUTMANAGER_FILES} ${PSP_KERNELBUTTON_FILES} ${PSP_MEDIAENGINEFILES} ${PSP_UI_FILES} ${PSP_UTILITY_FILES})
				Set(PSPSDK_LIBS -lpspdebug -lpspdisplay -lpspge -lpspctrl -lpspsdk -lpsplibc -lpsputility -lpspuser -lpspkernel )


				#Handle .S files - Probably a better way to do this
				set_property(SOURCE SysPSP/HLEGraphics/ConvertVertices.S PROPERTY LANGUAGE C)
				set_property(SOURCE SysPSP/DynaRec/DynaRecStubs.S PROPERTY LANGUAGE C)
				set_property(SOURCE SysPSP/HLEGraphics/TnLVFPU.S PROPERTY LANGUAGE C)
				set_property(SOURCE SysPSP/HLEGraphics/VectorClipping.S PROPERTY LANGUAGE C)
				set_property(SOURCE SysPSP/Utility/AtomicPrimitives.S PROPERTY LANGUAGE C)
				set_property(SOURCE SysPSP/Utility/DisableFPUExceptions.S PROPERTY LANGUAGE C)
				set_property(SOURCE SysPSP/Debug/mcount.S PROPERTY LANGUAGE C)
				set_property(SOURCE SysPSP/PRX/DveMgr/pspDveManager.S PROPERTY LANGUAGE C)
				set_property(SOURCE SysPSP/PRX/KernelButtons/imposectrl.S PROPERTY LANGUAGE C)
				set_property(SOURCE SysPSP/PRX/MediaEngine/MediaEngine.S PROPERTY LANGUAGE C)


				#SysGL
				set (SYSGL_GRAPHICS SysGL/Graphics/GraphicsContextGL.cpp SysGL/Graphics/NativeTextureGL.cpp)
				set (SYSGL_HLEGRAPHICS SysGL/HLEGraphics/GraphicsPluginGL.cpp SysGL/HLEGraphics/RendererGL.cpp)
				set (SYSGL_INPUT SysGL/Input/InputManagerGL.cpp)
				set (SYSGL_INTERFACE SysGL/Interface/UI.cpp)
				set (SYSGL_BUILD ${SYSGL_GRAPHICS} ${SYSGL_HLEGRAPHICS} ${SYSGL_INPUT} ${SYSGL_INTERFACE} ${PLUGIN_FILES})

				#Windows -- Not sure if it works yet
				set (WIN_AUDIO SysW32/HLEAudio/AudioPluginW32.cpp)
				set (WIN_DEBUG SysW32/Debug/DaedalusAssertW32.cpp SysW32/Debug/DebugConsoleW32.cpp)
				set (WIN_UTILITY SysW32/Utility/IOW32.cpp SysW32/Utility/ThreadW32.cpp SysW32/Utility/TimingW32.cpp)
				set (WIN_BUILD ${WIN_AUDIO} ${WIN_DEBUG} ${WIN_UTILITY})

        #Posix
				set (POSIX_DEBUG SysPosix/Debug/DaedalusAssertPosix.cpp SysPosix/Debug/DebugConsolePosix.cpp SysPosix/Debug/WebDebug.cpp SysPosix/Debug/WebDebugTemplate.cpp)
				set (POSIX_DYNAREC SysPosix/DynaRec/CodeBufferManagerPosix.cpp)
				set (POSIX_HLEGRAPHICS SysPosix/HLEGraphics/DisplayListDebugger.cpp)
				set (POSIX_MAIN_FILES SysPosix/main.cpp)
				set (POSIX_UTILITY SysPosix/Utility/CondPosix.cpp SysPosix/Utility/IOPosix.cpp SysPosix/Utility/ThreadPosix.cpp SysPosix/Utility/TimingPosix.cpp)
				set (POSIX_BUILD ${POSIX_DEBUG} ${POSIX_DYNAREC} ${POSIX_HLEGRAPHICS} ${POSIX_UTILITY})

				# These will remain separate for now..
				set (LINUX_AUDIO SysPosix/HLEAudio/AudioPluginLinux.cpp)
				set (MAC_AUDIO SysPosix/HLEAudio/AudioPluginOSX.cpp)


if (PSP_RELEASE)
	message("PSP Release Build..")
	add_definitions("-DNDEBUG -O3 ")
	add_compile_options(-DPSP_LARGE_MEMORY=1 -DPSP_FW_VERSION=661)
	add_definitions(-DDAEDALUS_PSP)
	include_directories(${PROJECT_SOURCE_DIR}/Config/Release)
	add_library(pspkubridge STATIC IMPORTED)
set_property(TARGET pspkubridge PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/SysPSP/Include/lib/libpspkubridge.a)
	include_directories(BEFORE ${PROJECT_SOURCE_DIR}/SysPSP/Include)
	add_library(daedalus STATIC ${BUILD} ${PSP_BUILD})
	add_executable(daedalus.elf ${PSP_MAIN_FILES})
	set_target_properties(daedalus.elf PROPERTIES LINK_FLAGS "-specs=${PSPSDK}/lib/prxspecs -Wl,-q,-T${PSPSDK}/lib/linkfile.prx ${PSPSDK}/lib/prxexports.o -L${PSPSDK}/lib")
	target_include_directories(daedalus.elf PUBLIC /usr/local/pspdev/psp/sdk/include )
	target_link_libraries(daedalus.elf daedalus -lstdc++ -lpsppower -lpspgu -lpspaudio -lpsprtc -lpng -lz -lc -lg -lm -lpspfpu pspkubridge ${PSPSDK_LIBS})
endif (PSP_RELEASE)



#Only difference between PSP Release and Debug is the definitions and Config Dev File - Try and merge

if (PSP_DEBUG)
	message("PSP Debug Build..")
	include_directories(${PROJECT_SOURCE_DIR}/Config/Dev)
	add_definitions("-g -O2 -fno-omit-frame-pointer -G0 -D_DEBUG -MD -Wall")
	add_compile_options(-DPSP_FW_VERSION=661)
	add_definitions(-DDAEDALUS_PSP )
	add_library(pspkubridge STATIC IMPORTED)
set_property(TARGET pspkubridge PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/SysPSP/Include/lib/libpspkubridge.a)
	include_directories(BEFORE ${PROJECT_SOURCE_DIR}/SysPSP/Include)
	add_library(daedalus STATIC ${BUILD} ${PSP_BUILD} ${DEBUG_ONLY})
	add_executable(daedalus.elf ${PSP_MAIN_FILES})
	set_target_properties(daedalus.elf PROPERTIES LINK_FLAGS "-specs=${PSPSDK}/lib/prxspecs -Wl,-q,-T${PSPSDK}/lib/linkfile.prx ${PSPSDK}/lib/prxexports.o -L${PSPSDK}/lib")
	target_include_directories(daedalus.elf PUBLIC /usr/local/pspdev/psp/sdk/include )
	target_link_libraries(daedalus.elf daedalus -lstdc++ -lpsppower -lpspgu -lpspaudio -lpsprtc -lpng -lz -lc -lg -lm -lpspfpu pspkubridge ${PSPSDK_LIBS})
endif (PSP_DEBUG)

if (MAC_RELEASE OR LINUX_RELEASE)

add_definitions("-DNDEBUG -O3")
include_directories(${PROJECT_SOURCE_DIR/HLEGraphics})
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/Config/Release ${PROJECT_SOURCE_DIR}/SysPosix/Include)

endif (MAC_RELEASE OR LINUX_RELEASE)

if (LINUX_RELEASE)


		message("Linux Release Build..")

		#Build SysGL Lib
		add_library(sysGL STATIC ${SYSGL_BUILD})
		target_link_libraries(sysGL GL GLEW -lSDL2 dl X11  )

		#Build Daedalus Lib
		add_library(daedalus.lib STATIC ${BUILD} ${POSIX_BUILD} ${LINUX_AUDIO} )
	target_link_libraries(daedalus.lib sysGL -lGL  -lSDL2 -lGLEW png z minizip pthread)

		#Build and Link Executable
			add_executable(daedalus ${POSIX_MAIN_FILES})
		target_link_libraries(daedalus LINK_PUBLIC daedalus.lib )
endif (LINUX_RELEASE)


if (MAC_RELEASE)

		message("Mac Release Build..")
FIND_PATH(OPENGL_INCLUDE_DIR gl.h)
	FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		include_directories(/usr/local/include -framework OpenGL)
		link_directories(/usr/local/lib)

    add_definitions("-DDAEDALUS_OSX" -g)
		#Build SysGL Lib
		add_library(sysGL STATIC ${SYSGL_BUILD})
		target_link_libraries(sysGL -lGLEW -lSDL2 -ldl "-framework OpeNGL -framework AudioToolbox -framework Cocoa -framework IOKit -framework CoreVideo"  )

		#Build Daedalus Lib
		add_library(daedalus.lib STATIC ${BUILD} ${POSIX_BUILD} ${MAC_AUDIO} )
	target_link_libraries(daedalus.lib sysGL png z minizip pthread)

		#Build and Link Executable
			add_executable(daedalus ${POSIX_MAIN_FILES})
		target_link_libraries(daedalus LINK_PUBLIC daedalus.lib )
endif (MAC_RELEASE)
