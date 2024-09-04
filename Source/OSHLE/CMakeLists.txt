add_library(OSHLE OBJECT 
                        OS.cpp
                        patch.cpp
                        )

if(DAEDALUS_PROFILE_OSHLE)
target_compile_options(OSHLE PRIVATE -pg)
endif(DAEDALUS_PROFILE_OSHLE)

list(APPEND daed_libs "OSHLE")
set(daed_libs ${daed_libs} PARENT_SCOPE)
