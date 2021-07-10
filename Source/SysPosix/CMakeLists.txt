
				##Posix Definitions
				include_directories(BEFORE /usr/local/)
				link_directories(/usr/local/lib)
				
				add_library(plat_dbg OBJECT Debug/DaedalusAssertPosix.cpp Debug/DebugConsolePosix.cpp Debug/WebDebug.cpp Debug/WebDebugTemplate.cpp HLEGraphics/DisplayListDebugger.cpp)
				add_library(plat_dynarec OBJECT DynaRec/CodeBufferManagerPosix.cpp)
				add_library(plat_utility OBJECT Utility/CondPosix.cpp Utility/IOPosix.cpp Utility/ThreadPosix.cpp Utility/TimingPosix.cpp)

				## Audio options
									add_definitions(-DDAEDALUS_POSIX -DDAEDALUS_GL)
				if(APPLE)
				add_library(plat_audio OBJECT HLEAudio/AudioPluginOSX.cpp)
				elseif(UNIX AND NOT APPLE)
				add_library(plat_audio HLEAudio/AudioPluginLinux.cpp)
				endif(APPLE)
