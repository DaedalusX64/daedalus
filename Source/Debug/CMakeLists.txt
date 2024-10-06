add_library(Debug OBJECT
                DebugConsoleImpl.cpp 
                DebugLog.cpp
                Dump.cpp
                PrintOpCode.cpp
                Registers.cpp
                Synchroniser.cpp
            )

 if(DAEDALUS_PROFILE_DEBUG)
target_compile_options(Debug PRIVATE -pg)
endif(DAEDALUS_PROFILE_DEBUG)

list(APPEND daed_libs "Debug")
set(daed_libs ${daed_libs} PARENT_SCOPE)
