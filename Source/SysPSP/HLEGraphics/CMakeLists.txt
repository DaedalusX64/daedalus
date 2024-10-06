add_library(HLEGraphics_PSP OBJECT 
                                    Blender.cpp 
                                    BlendModes.cpp 
                                    ColourAdjuster.cpp 
                                    ConvertVertices.S 
                                    DisplayListDebugger.cpp 
                                    GraphicsPluginPSP.cpp 
                                    RendererPSP.cpp 
                                    TnLVFPU.S 
                                    TransformWithColour.S
                                    TransformWithLighting.S
                                    VectorClipping.S

                                    Combiner/BlendConstant.cpp 
                                    Combiner/CombinerExpression.cpp 
                                    Combiner/CombinerTree.cpp 
                                    Combiner/RenderSettings.cpp
            )

if(DAEDALUS_PROFILE_HLEGRAPHICS_PSP)
    target_compile_options(HLEGraphics_PSP PRIVATE -pg)
endif(DAEDALUS_PROFILE_HLEGRAPHICS_PSP)

list(APPEND daed_libs "HLEGraphics_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
