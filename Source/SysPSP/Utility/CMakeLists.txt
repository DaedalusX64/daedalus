add_library(Utility_PSP OBJECT 
                                AtomicPrimitives.S 
                                BatteryPSP.cpp 
                                DebugMemory.cpp 
                                DisableFPUExceptions.S 
                                exception.cpp 
                                FastMemcpyPSP.cpp 
                                ModulePSP.cpp
                                VolatileMemPSP.cpp
                                ${PROJECT_SOURCE_DIR}/Source/SysPosix/Utility/TimingPosix.cpp 
                                ${PROJECT_SOURCE_DIR}/Source/SysPosix/Utility/ThreadPosix.cpp
                                ${PROJECT_SOURCE_DIR}/Source/SysPosix/Utility/IOPosix.cpp
            )

            
list(APPEND daed_libs "Utility_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
