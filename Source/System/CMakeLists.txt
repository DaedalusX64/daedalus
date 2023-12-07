add_library(System OBJECT CompressedStream.cpp DataSink.cpp SystemInit.cpp)

target_include_directories(System PUBLIC ${ZLIB_INCLUDE_DIRS})

list(APPEND daed_libs "System")
set(daed_libs ${daed_libs} PARENT_SCOPE)
