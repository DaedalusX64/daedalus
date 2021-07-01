
				##Posix Definitions
				add_definitions(-DDAEDALUS_POSIX)
				add_library(plat_dbg Debug/DaedalusAssertPosix.cpp Debug/DebugConsolePosix.cpp Debug/WebDebug.cpp Debug/WebDebugTemplate.cpp HLEGraphics/DisplayListDebugger.cpp)
				add_library(plat_dynarec DynaRec/CodeBufferManagerPosix.cpp)
				add_library(plat_utility Utility/CondPosix.cpp Utility/IOPosix.cpp Utility/ThreadPosix.cpp Utility/TimingPosix.cpp)

				## Audio options

				if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
				add_library(plat_audio HLEAudio/AudioPluginOSX.cpp)
				elseif(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
				add_library(plat_audio HLEAudio/AudioPluginLinux.cpp)
				endif(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)


				set (POSIX_MAIN_FILES main.cpp)