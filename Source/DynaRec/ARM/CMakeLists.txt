add_library(DynaRec_ARM OBJECT 
                AssemblyUtilsARM.cpp
                AssemblyWriterARM.cpp
                CodeBufferManagerARM.cpp
                CodeGeneratorARM.cpp  
                DynarecStubARM.S
                N64RegisterCacheARM.cpp
)

list(APPEND daed_libs "DynaRec_ARM")
set(daed_libs ${daed_libs} PARENT_SCOPE)
