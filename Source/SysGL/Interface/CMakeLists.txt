add_library(Interface_GL OBJECT UI.cpp  
##  This will need to fixed later for different variants but for now OGL3 is what we're using for Linux and macOS
                                ${PROJECT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_opengl3.cpp
            )

