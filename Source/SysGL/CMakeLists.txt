file(GLOB SRC_FILES Graphics/*.cpp HLEGraphics/*.cpp)

add_library(SysGL OBJECT ${SRC_FILES})

target_include_directories(SysGL PUBLIC ${OPENGL_INCLUDE_DIR})
target_include_directories(SysGL PUBLIC ${GLEW_INCLUDE_DIRS})
target_include_directories(SysGL PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(SysGL PUBLIC ${SDL2_TTF_INCLUDE_DIRS})

install(FILES ${PROJECT_SOURCE_DIR}/Source/SysGL/HLEGraphics/n64.psh DESTINATION DaedalusX64)

if(DAEDALUS_PROFILE_GRAPHICS_GL)
target_compile_options(SysGL PRIVATE -pg)
endif(DAEDALUS_PROFILE_GRAPHICS_GL)

list(APPEND daed_libs "SysGL")
set(daed_libs ${daed_libs} PARENT_SCOPE)
