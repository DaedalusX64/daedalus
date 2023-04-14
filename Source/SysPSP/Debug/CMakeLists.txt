add_library(Debug_PSP OBJECT 
                            DaedalusAssertPSP.cpp 
                            DBGConsolePSP.cpp 
            )

            set (GPROF_SRCS SysPSP/Debug/prof.c SysPSP/Debug/mcount.S )

            
list(APPEND daed_libs "Debug_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
