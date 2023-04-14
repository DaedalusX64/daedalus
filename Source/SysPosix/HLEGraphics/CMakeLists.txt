add_library(HLEGraphics_POSIX OBJECT 
                                    DisplayListDebugger.cpp
            )

list(APPEND daed_libs HLEGraphics_POSIX)
set(daed_libs ${daed_libs} PARENT_SCOPE)

