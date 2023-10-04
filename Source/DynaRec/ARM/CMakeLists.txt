add_library(DynaRec_ARM OBJECT 
                CodeGeneratorARM.cpp  
                AssemblyUtilsARM.cpp
                AssemblyWriterARM.cpp
                CodeBufferManagerARM.cpp
                              
                DynarecStubARM.S
                N64RegisterCacheARM.cpp
)

list(APPEND daed_libs "DynaRec_ARM")
set(daed_libs ${daed_libs} PARENT_SCOPE)
