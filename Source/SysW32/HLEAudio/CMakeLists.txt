add_library(HLEAudio_Win OBJECT 
                            AudioPluginW32.cpp
            )

list(APPEND daed_libs HLEAudio_Win)
set(daed_libs ${daed_libs} PARENT_SCOPE)

