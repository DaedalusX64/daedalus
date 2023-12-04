add_library(X86_Dynarec OBJECT 
                                AssemblyUtilsX86.cpp
                                AssemblyWriterX86.cpp
                                CodeBufferManagerX86.cpp
                                CodeGeneratorX86.cpp                    
            )

            
list(APPEND daed_libs "X86_Dynarec")
set(daed_libs ${daed_libs} PARENT_SCOPE)