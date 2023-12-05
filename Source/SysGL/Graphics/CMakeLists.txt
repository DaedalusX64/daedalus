add_library(Graphics_GL OBJECT GraphicsContextGL.cpp NativeTextureGL.cpp UIContext.cpp)


list(APPEND include_dirs ${PROJECT_SOURCE_DIR}/Source/third_party/imgui)
set(include_dirs ${include_dirs} PARENT_SCOPE)

list(APPEND daed_libs Graphics_GL)
set(daed_libs ${daed_libs} PARENT_SCOPE)

