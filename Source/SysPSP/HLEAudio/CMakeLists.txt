add_library(HLEAudio_PSP OBJECT 
                            AudioPluginPSP.cpp
             )

list(APPEND daed_libs "HLEAudio_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
             