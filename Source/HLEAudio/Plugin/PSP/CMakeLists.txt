if(DAEDALUS_SDL)
add_library(HLEAudio_PSP OBJECT ${PROJECT_SOURCE_DIR}/Source/SysPosix/HLEAudio/AudioPluginSDL.cpp)
else()
add_library(HLEAudio_PSP OBJECT AudioPluginPSP.cpp)
endif()

list(APPEND daed_libs "HLEAudio_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
             