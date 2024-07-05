add_library(Utility_Win OBJECT
                                CondW32.cpp
                                DISASM86.cpp
                               ${PROJECT_SOURCE_DIR}/Source/SysPosix/Utility/IOPosix.cpp
                                ThreadW32.cpp
                                ${PROJECT_SOURCE_DIR}/Source/SysPosix/Utility/TimingPosix.cpp
            )

list(APPEND daed_libs Utility_Win)
set(daed_libs ${daed_libs} PARENT_SCOPE)

