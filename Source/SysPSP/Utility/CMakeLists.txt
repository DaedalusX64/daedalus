add_library(Utility_PSP OBJECT 
                                AtomicPrimitives.S 
                                BatteryPSP.cpp 
                                Buttons.cpp 
                                DebugMemory.cpp 
                                DisableFPUExceptions.S 
                                exception.cpp 
                                FastMemcpyPSP.cpp 
                                JobManager.cpp 
                                ModulePSP.cpp 
                                TimingPSP.cpp 
                                Translate.cpp 
                                VolatileMemPSP.cpp
                                ${PROJECT_SOURCE_DIR}/SysPosix/Utility/ThreadPosix.cpp
                                ${PROJECT_SOURCE_DIR}/SysPosix/Utility/IOPosix.cpp
            )
