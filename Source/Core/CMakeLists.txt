add_library(Core OBJECT 
                        HvqmTask.cpp 
                        RDRam.cpp 
                        Cheats.cpp 
                        CPU.cpp 
                        DMA.cpp 
                        Dynamo.cpp 
                        FlashMem.cpp  
                        FramerateLimiter.cpp 
                        Interpret.cpp 
                        Interrupt.cpp 
                        JpegTask.cpp 
                        Memory.cpp 
                        PIF.cpp 
                        PrintOpCode.cpp 
                        R4300.cpp 
                        Registers.cpp
                        ROM.cpp
                        ROMBuffer.cpp 
                        ROMImage.cpp 
                        RomSettings.cpp 
                        RSP_HLE.cpp 
                        Save.cpp 
                        TLB.cpp
                        )

# Cannot compile with -O2 or above on Apple Intel
if(APPLE)
set_source_files_properties(R4300.cpp PROPERTIES COMPILE_FLAGS -O0)
endif(APPLE)

list(APPEND daed_libs "Core")
set(daed_libs ${daed_libs} PARENT_SCOPE)
