add_library(Dynarec_POSIX OBJECT 
                                CodeBufferManagerPosix.cpp
            )

list(APPEND daed_libs Dynarec_POSIX)
set(daed_libs ${daed_libs} PARENT_SCOPE)

