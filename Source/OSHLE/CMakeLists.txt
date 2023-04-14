add_library(OSHLE OBJECT 
                        OS.cpp
                        patch.cpp
                        )

                        
list(APPEND daed_libs "OSHLE")
set(daed_libs ${daed_libs} PARENT_SCOPE)
