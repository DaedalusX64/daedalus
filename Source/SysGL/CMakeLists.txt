
file(GLOB SRC_FILES Graphics/*.cpp HLEGraphics/*.cpp Input/*.cpp Interface/*.cpp)

add_library(SysGL OBJECT ${SRC_FILES})

find_package(OpenGL REQUIRED)
target_include_directories(SysGL PUBLIC ${OPENGL_INCLUDE_DIR})
link_libraries(OpenGL::GL)

find_package(GLEW REQUIRED)
target_include_directories(SysGL PUBLIC ${GLEW_INCLUDE_DIRS})
link_libraries(GLEW::GLEW)

find_package(SDL2 CONFIG REQUIRED)
target_include_directories(SysGL PUBLIC ${SDL2_INCLUDE_DIRS})
link_libraries(SDL2::SDL2)

# PSP is not needed as it uses built-in native font
find_package(SDL2_ttf REQUIRED)
target_include_directories(SysGL PUBLIC ${SDL2_TTF_INCLUDE_DIRS})
link_libraries(SDL2_ttf::SDL2_ttf)

install(FILES ${PROJECT_SOURCE_DIR}/Source/SysGL/HLEGraphics/n64.psh DESTINATION DaedalusX64)

list(APPEND daed_libs "SysGL")
set(daed_libs ${daed_libs} PARENT_SCOPE)
