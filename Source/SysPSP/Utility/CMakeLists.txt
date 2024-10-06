add_library(Utility_PSP OBJECT 
                                AtomicPrimitives.S 
                                BatteryPSP.cpp 
                                DebugMemory.cpp 
                                # DisableFPUExceptions.S 
                                exception.cpp 
                                FastMemcpyPSP.cpp 
                                ModulePSP.cpp
                                VolatileMemPSP.cpp              
            )

    if(DAEDALUS_PROFILE_UTILITY_PSP)
        target_compile_options(RomFile PRIVATE -pg)
endif(DAEDALUS_PROFILE_UTILITY_PSP)
            

            
list(APPEND daed_libs "Utility_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
