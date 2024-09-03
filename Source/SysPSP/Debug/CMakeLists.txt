add_library(Debug_PSP OBJECT 
                            DaedalusAssertPSP.cpp 
                            DBGConsolePSP.cpp 
            )

            
list(APPEND daed_libs "Debug_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
