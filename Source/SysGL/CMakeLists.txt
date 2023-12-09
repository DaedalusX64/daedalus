file(GLOB SRC_FILES Graphics/*.cpp HLEGraphics/*.cpp Input/*.cpp Interface/*.cpp)

add_library(SysGL OBJECT ${SRC_FILES})

target_include_directories(SysGL PUBLIC ${OPENGL_INCLUDE_DIR})
target_include_directories(SysGL PUBLIC ${GLEW_INCLUDE_DIRS})
target_include_directories(SysGL PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(SysGL PUBLIC ${SDL2_TTF_INCLUDE_DIRS})

install(FILES ${PROJECT_SOURCE_DIR}/Source/SysGL/HLEGraphics/n64.psh DESTINATION DaedalusX64)

list(APPEND daed_libs "SysGL")
set(daed_libs ${daed_libs} PARENT_SCOPE)
