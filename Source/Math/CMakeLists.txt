add_library(Math OBJECT Matrix4x4.cpp)

if(DAEDALUS_PROFILE_MATH)
target_compile_options(Math PRIVATE -pg)
endif(DAEDALUS_PROFILE_MATH)

list(APPEND daed_libs "Math")
set(daed_libs ${daed_libs} PARENT_SCOPE)
