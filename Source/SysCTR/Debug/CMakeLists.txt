add_library(Debug_CTR OBJECT 
                    DaedalusAssertCTR.cpp
                    DBGConsoleCTR.cpp
            )

            
list(APPEND daed_libs "Debug_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)
