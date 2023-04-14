

    ## This will need to be fixed for all platforms for proper detection
    if (UNIX)
    # set(BACKEND ${PROJECT_SOURCE_DIR}/Source/third_party/imgui/backends/imgui_impl_SDL.mm) ?
    set(BACKEND ${PROJECT_SOURCE_DIR}/Source/third_party/imgui/backends/imgui_impl_opengl3.cpp)
    elseif(CTR)
    set(BACKEND ${PROJECT_SOURCE_DIR}/Source/third_party/imgui/backends/imgui_impl_3DS.cpp)
    endif()

    
add_library(imgui OBJECT
            ${PROJECT_SOURCE_DIR}/Source/third_party/imgui/imgui.cpp 
            ${PROJECT_SOURCE_DIR}/Source/third_party/imgui/imgui_demo.cpp 
            ${PROJECT_SOURCE_DIR}/Source/third_party/imgui/imgui_draw.cpp 
            ${PROJECT_SOURCE_DIR}/Source/third_party/imgui/imgui_tables.cpp
            ${PROJECT_SOURCE_DIR}/Source/third_party/imgui/imgui_widgets.cpp 
            ${PROJECT_SOURCE_DIR}/Source/third_party/imgui/backends/imgui_impl_sdl.cpp 
            ${BACKEND}
            )       