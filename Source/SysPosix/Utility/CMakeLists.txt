add_library(Utility_POSIX OBJECT 
                                CondPosix.cpp 
                                IOPosix.cpp 
                                ThreadPosix.cpp 
                                TimingPosix.cpp
            )

list(APPEND daed_libs Utility_POSIX)
set(daed_libs ${daed_libs} PARENT_SCOPE)

