add_library(Math OBJECT Matrix4x4.cpp)


list(APPEND daed_libs "Math")
set(daed_libs ${daed_libs} PARENT_SCOPE)
