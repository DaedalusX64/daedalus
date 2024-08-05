if(DAEDALUS_PSP)
add_library(Input OBJECT PSP/InputManagerPSP.cpp)
elseif(DAEDALUS_CTR)
add_library(Input OBJECT CTR/InputManagerCTR.cpp)
else()
add_library(Input OBJECT SDL/InputManagerSDL.cpp)
endif()

list(APPEND daed_libs "Input")
set(daed_libs ${daed_libs} PARENT_SCOPE)
             