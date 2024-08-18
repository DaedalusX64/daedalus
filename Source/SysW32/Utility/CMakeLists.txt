add_library(Utility_Win OBJECT
                                CondW32.cpp
                                ThreadW32.cpp
            )

            if(DEBUG)
            add_library(Utility_Win OBJECT
            DISASM86.cpp
                        )
            list(APPEND daed_libs Utility_Win)
         endif()
list(APPEND daed_libs Utility_Win)
set(daed_libs ${daed_libs} PARENT_SCOPE)

