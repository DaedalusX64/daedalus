file(GLOB SRC_FILES *.cpp)

add_library(DCGraphics OBJECT ${SRC_FILES})


if(DAEDALUS_PROFILE_GRAPHICS_GL)
target_compile_options(SysGLES PRIVATE -pg)
endif(DAEDALUS_PROFILE_GRAPHICS_GL)

list(APPEND daed_libs "Graphics")
set(daed_libs ${daed_libs} PARENT_SCOPE)
