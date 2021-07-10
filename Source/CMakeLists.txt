				add_library(config OBJECT Config/ConfigOptions.cpp)
				add_library(core OBJECT Core/HvqmTask.cpp Core/RDRam.cpp Core/Cheats.cpp Core/CPU.cpp Core/DMA.cpp Core/Dynamo.cpp Core/FlashMem.cpp  Core/FramerateLimiter.cpp Core/Interpret.cpp Core/Interrupt.cpp Core/JpegTask.cpp Core/Memory.cpp Core/PIF.cpp Core/PrintOpCode.cpp Core/R4300.cpp Core/Registers.cpp Core/ROM.cpp Core/ROMBuffer.cpp Core/ROMImage.cpp Core/RomSettings.cpp Core/RSP_HLE.cpp Core/Save.cpp Core/TLB.cpp)
				add_library(dbg OBJECT Debug/DebugConsoleImpl.cpp Debug/DebugLog.cpp Debug/Dump.cpp Debug/Synchroniser.cpp)
				add_library(dynarec OBJECT DynaRec/BranchType.cpp DynaRec/DynaRecProfile.cpp DynaRec/Fragment.cpp DynaRec/FragmentCache.cpp DynaRec/IndirectExitMap.cpp DynaRec/StaticAnalysis.cpp DynaRec/TraceRecorder.cpp )
				add_library(graphics OBJECT Graphics/ColourValue.cpp Graphics/PngUtil.cpp Graphics/TextureTransform.cpp)
				add_library(hleaudio OBJECT HLEAudio/HLEAudioState.cpp HLEAudio/ABI1.cpp HLEAudio/ABI2.cpp HLEAudio/ABI3.cpp HLEAudio/ABI3mp3.cpp HLEAudio/AudioBuffer.cpp HLEAudio/HLEMain.cpp HLEAudio/ABI_ADPCM.cpp HLEAudio/ABI_Buffers.cpp HLEAudio/ABI_Filters.cpp HLEAudio/ABI_MixerInterleave.cpp HLEAudio/ENV_Mixer.cpp HLEAudio/ABI_Resample.cpp)
				add_library(hlegraphics OBJECT HLEGraphics/BaseRenderer.cpp HLEGraphics/BaseRenderer.h HLEGraphics/CachedTexture.cpp HLEGraphics/ConvertImage.cpp HLEGraphics/ConvertTile.cpp HLEGraphics/DLDebug.cpp HLEGraphics/DLParser.cpp HLEGraphics/Microcode.cpp HLEGraphics/RDP.cpp HLEGraphics/RDPStateManager.cpp HLEGraphics/TextureCache.cpp HLEGraphics/TextureCacheWebDebug.cpp HLEGraphics/TextureInfo.cpp HLEGraphics/TMEM.cpp HLEGraphics/uCodes/Ucode.cpp)
				add_library(interface OBJECT Interface/RomDB.cpp Interface/Preferences.cpp Interface/SaveState.cpp )
				add_library(math OBJECT Math/Matrix4x4.cpp)
				add_library(oshle OBJECT OSHLE/OS.cpp OSHLE/patch.cpp)
				add_library(romfile OBJECT RomFile/RomFile.cpp RomFile/RomFileCache.cpp RomFile/RomFileCompressed.cpp RomFile/RomFileMemory.cpp RomFile/RomFileUncompressed.cpp)
				add_library(system OBJECT System/CompressedStream.cpp System/CompressedStream.cpp System/DataSink.cpp System/Paths.cpp System/SystemInit.cpp )
				add_library(test OBJECT Test/BatchTest.cpp)
				add_library(utility OBJECT Utility/CRC.cpp Utility/FastMemcpy.cpp Utility/Hash.cpp Utility/IniFile.cpp Utility/MemoryHeap.cpp  Utility/Profiler.cpp  Utility/Stream.cpp Utility/StringUtil.cpp Utility/Timer.cpp  Utility/Translate.cpp )
				# add_library(other OBJECT Utility/FastMemcpy_test.cpp )

				link_libraries(hlegraphics graphics plat_hlegraphics plat_ui)
				link_libraries(oshle dbg)
				link_libraries(plat_graphics plat_audio plat_input plat_utility )
 				link_libraries(config core hleaudio interface math oshle romfile system test utility)
				
				if(PSP)
					link_libraries(dynarec plat_dynarec)  ## PSP is only platform that has dynarec ATM
				endif(PSP)
				
			
find_package(PNG REQUIRED)

if(UNIX)
include_directories(/usr/local/include ${OPENGL_INCLUDE_DIR})
link_directories(/usr/local/lib)

add_executable(daedalus SysPosix/main.cpp)
target_link_libraries(daedalus -lminizip -lpthread 
						${PNG_LIBRARY} ${ZLIB_LIBRARY} 
					)

elseif(WIN32)
add_compile_definitions(DAEDALUS_WIN32
						DAEDALUS_GL)
elseif(PSP)
	add_executable(daedalus SysPSP/main.cpp)
	target_link_libraries(daedalus -lpsppower -lpspgu -lpspaudiolib -lpspaudio -lpsprtc -lg -lm -lpspfpu -lpspkubridge psp_plugins ${PSPSDK_LIBS} ${PNG_LIBRARY} ${ZLIB_LIBRARY})
	create_pbp_file( TARGET daedalus 
					TITLE "DaedalusX64" 
					ICON_PATH "${CMAKE_SOURCE_DIR}/Source/SysPSP/Resources/eboot_icons/icon0.png" 
					PREVIEW_PATH "${CMAKE_SOURCE_DIR}/Source/SysPSP/Resources/eboot_icons/pic1.png"
					)
endif(UNIX)

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

	#Build SysGL
	add_library(sysGL STATIC ${SYSGL_BUILD})
	target_link_libraries(sysGL OpenGL32 glfw3 glew32s SDL2 Xinput ${OS_LIBS})

	#Build Daedalus Lib
	add_library(daedalus.lib STATIC ${BUILD} ${WIN_BUILD} ${AUDIO_DRIVER} )
	target_link_libraries(daedalus.lib dsound dxguid sysGL libpng zlib zlibwapi)

	#Build and Link Executable
	add_executable(daedalus SysWin32/main.cpp)
	target_link_libraries(daedalus LINK_PUBLIC daedalus.lib Shlwapi.lib)


endif (WIN32)