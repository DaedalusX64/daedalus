file(GLOB SRC_FILES *.cpp)

add_library(DCUtility OBJECT ${SRC_FILES})


list(APPEND daed_libs "Utility")
set(daed_libs ${daed_libs} PARENT_SCOPE)
