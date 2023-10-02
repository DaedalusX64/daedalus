add_library(HLEAudio_PSP OBJECT 
                            # ${PROJECT_SOURCE_DIR}/Source/SysPosix/HLEAudio/AudioPluginSDL.cpp
                            AudioPluginPSP_SDL.cpp
                            )

list(APPEND daed_libs "HLEAudio_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
             