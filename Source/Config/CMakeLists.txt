add_library(Config OBJECT ConfigOptions.cpp)

list(APPEND daed_libs "Config")
set(daed_libs ${daed_libs} PARENT_SCOPE)
