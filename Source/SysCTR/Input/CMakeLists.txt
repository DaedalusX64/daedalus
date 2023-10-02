
add_library(Input_CTR OBJECT
            InputManagerCTR.cpp
            )

            
list(APPEND daed_libs "Input_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)
