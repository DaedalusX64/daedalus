add_library(Core OBJECT 


                        CPU.cpp 
                        DMA.cpp 
                        Dynamo.cpp
                        FlashMem.cpp  
                        Interpret.cpp 
                        Interrupt.cpp 
                        Memory.cpp 
                        PIF.cpp 
                        R4300.cpp
                        RDRam.cpp 
                        ROM.cpp
                        ROMImage.cpp 
                        RSP_HLE.cpp 
                        Save.cpp 
                        TLB.cpp
                        )

# Cannot compile with -O2 or above on Apple Intel
if(APPLE)
set_source_files_properties(R4300.cpp PROPERTIES COMPILE_FLAGS -O0)
endif(APPLE)

if(DAEDALUS_PROFILE_CORE)
target_compile_options(Core PRIVATE -pg)
endif(DAEDALUS_PROFILE_CORE)

list(APPEND daed_libs "Core")
set(daed_libs ${daed_libs} PARENT_SCOPE)
