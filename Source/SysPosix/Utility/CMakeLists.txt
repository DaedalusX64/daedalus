add_library(Utility_POSIX OBJECT 
                                CondPosix.cpp 
                                ThreadPosix.cpp 
            )

if(DAEDALUS_PROFILE_UTILITY_POSIX)
    target_compile_options(Utility_POSIX PRIVATE -pg)
endif(DAEDALUS_PROFILE_UTILITY_POSIX)
            

list(APPEND daed_libs Utility_POSIX)
set(daed_libs ${daed_libs} PARENT_SCOPE)

