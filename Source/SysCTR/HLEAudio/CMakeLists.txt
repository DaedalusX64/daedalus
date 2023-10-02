add_library(HLEAudio_CTR OBJECT 
            AudioOutput.cpp
            AudioPluginCTR.cpp
                    
    )

            
list(APPEND daed_libs "HLEAudio_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)
