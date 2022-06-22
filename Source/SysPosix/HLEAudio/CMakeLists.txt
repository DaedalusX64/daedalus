if(UNIX AND NOT APPLE)
add_library(HLEAudio_Plat OBJECT AudioPluginSDL.cpp)
elseif( APPLE)
add_library(HLEAudio_Plat OBJECT AudioPluginOSX.cpp)
else() ## No Audio available
add_library(HLEAudio_Plat OBJECT AudioPluginNone.cpp)
endif()
