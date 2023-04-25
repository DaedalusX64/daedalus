add_library(Debug OBJECT
                DebugConsoleImpl.cpp 
                DebugLog.cpp
                Dump.cpp
                Synchroniser.cpp
            )

list(APPEND daed_libs "Debug")
set(daed_libs ${daed_libs} PARENT_SCOPE)
