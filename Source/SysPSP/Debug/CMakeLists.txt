add_library(Debug_Plat OBJECT 
                            DaedalusAssertPSP.cpp 
                            DBGConsolePSP.cpp 
            )

            set (GPROF_SRCS SysPSP/Debug/prof.c SysPSP/Debug/mcount.S )
