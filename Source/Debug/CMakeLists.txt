add_library(Debug OBJECT
                DebugConsoleImpl.cpp 
                DebugLog.cpp
                Dump.cpp
                PrintOpCode.cpp
                Registers.cpp
                Synchroniser.cpp
            )

list(APPEND daed_libs "Debug")
set(daed_libs ${daed_libs} PARENT_SCOPE)
