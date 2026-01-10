file(GLOB SRC_FILES *.cpp)

add_library(DCHLEGraphics OBJECT ${SRC_FILES})


list(APPEND daed_libs "Graphics")
set(daed_libs ${daed_libs} PARENT_SCOPE)
