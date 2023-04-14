add_library(HLEGraphics_GL OBJECT 
                                    GraphicsPluginGL.cpp 
                                    RendererGL.cpp
            )        

list(APPEND daed_libs HLEGraphics_GL)
set(daed_libs ${daed_libs} PARENT_SCOPE)

