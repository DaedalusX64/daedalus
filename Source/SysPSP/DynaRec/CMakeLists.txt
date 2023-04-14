add_library(Dynarec_PSP OBJECT
                            AssemblyWriterPSP.cpp 
                            AssemblyUtilsPSP.cpp 
                            CodeBufferManagerPSP.cpp 
                            CodeGeneratorPSP.cpp 
                            DynaRecDebug.cpp
                            DynaRecStubs.S 
                            N64RegisterCachePSP.cpp
            )

            
list(APPEND daed_libs "Dynarec_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
