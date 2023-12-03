add_library(X64_Dynarec OBJECT 
                                AssemblyUtilsX64.cpp
                                CodeBufferManagerX64.cpp
                                CodeGeneratorX64.cpp                    
                                AssemblyWriterX64.cpp
            )

list(APPEND daed_libs "X64_Dynarec")
set(daed_libs ${daed_libs} PARENT_SCOPE)