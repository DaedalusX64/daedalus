
add_library(HLEGraphics_CTR OBJECT 
                    BlendModes.cpp
                    ColourAdjuster.cpp
                    GraphicsPluginCTR.cpp
                    RendererCTR.cpp
                    Combiner/BlendConstant.cpp
                    Combiner/CombinerExpression.cpp
                    Combiner/CombinerTree.cpp
                    Combiner/RenderSettings.cpp
                    )

if(DAEDALUS_PROFILE_HLEGRAPHICS_CTR)
     target_compile_options(HLEGraphics_CTR PRIVATE -pg)
 endif(DAEDALUS_PROFILE_HLEGRAPHICS_CTR)
                            

            
list(APPEND daed_libs "HLEGraphics_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)
