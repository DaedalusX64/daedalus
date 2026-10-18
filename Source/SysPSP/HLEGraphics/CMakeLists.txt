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

if(DAEDALUS_PSP_USE_ME)
    find_path(ME_CORE_INCLUDE_DIR me-core-mapper/me-core.h REQUIRED)
    find_library(ME_CORE_LIBRARY me-core REQUIRED)
    target_sources(HLEGraphics_PSP PRIVATE ../ME/MEGraphics.cpp)
    if(DAEDALUS_PSP_ME_AUDIO)
        target_sources(HLEGraphics_PSP PRIVATE ../ME/MEAudio.cpp ../ME/MEAudioInterrupt.S)
    endif()
    target_include_directories(HLEGraphics_PSP PRIVATE ${ME_CORE_INCLUDE_DIR})
    list(APPEND sys_libraries ${ME_CORE_LIBRARY})
    set(sys_libraries ${sys_libraries} PARENT_SCOPE)
endif()
