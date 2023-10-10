
add_library(HLEGraphics_CTR OBJECT 
                    BlendModes.cpp
                    ColourAdjuster.cpp
                    GraphicsPluginCTR.cpp
                    RendererCTR.cpp
                    )

                    add_subdirectory(Combiner)

            
list(APPEND daed_libs "HLEGraphics_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)
