add_library(Utility_Win OBJECT
                                CondW32.cpp
                                DISASM86.cpp
                                IOW32.cpp
                                ThreadW32.cpp
                                TimingW32.cpp
            )

list(APPEND daed_libs Utility_Win)
set(daed_libs ${daed_libs} PARENT_SCOPE)

