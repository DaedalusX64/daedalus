add_library(Debug_POSIX OBJECT 
                            DaedalusAssertPosix.cpp 
                            DebugConsolePosix.cpp 
                            WebDebug.cpp 
                            WebDebugTemplate.cpp 
                            ${PROJECT_SOURCE_DIR}/Source/third_party/webby/webby.c)

list(APPEND daed_libs Debug_POSIX)
set(daed_libs ${daed_libs} PARENT_SCOPE)

