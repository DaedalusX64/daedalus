add_library(Input_GL OBJECT 
                                InputManagerGL.cpp
            )

list(APPEND daed_libs Input_GL)
set(daed_libs ${daed_libs} PARENT_SCOPE)

