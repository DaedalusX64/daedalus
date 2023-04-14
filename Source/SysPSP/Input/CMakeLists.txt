add_library(Input_PSP OBJECT 
                                InputManagerPSP.cpp
            )

            
list(APPEND daed_libs "Input_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
