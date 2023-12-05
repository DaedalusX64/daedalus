add_library(Dynarec_Dummy OBJECT 
                                CodeBufferManagerDummy.cpp
                                AssemblyUtilsDummy.cpp
            )

list(APPEND daed_libs Dynarec_Dummy)
set(daed_libs ${daed_libs} PARENT_SCOPE)

