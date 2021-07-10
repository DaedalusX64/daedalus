enable_language(C)

set_property(SOURCE HLEGraphics/ConvertVertices.S PROPERTY LANGUAGE C)
set_property(SOURCE DynaRec/DynaRecStubs.S PROPERTY LANGUAGE C)
set_property(SOURCE HLEGraphics/TnLVFPU.S PROPERTY LANGUAGE C)
set_property(SOURCE HLEGraphics/VectorClipping.S PROPERTY LANGUAGE C)
set_property(SOURCE Utility/AtomicPrimitives.S PROPERTY LANGUAGE C)
set_property(SOURCE Utility/DisableFPUExceptions.S PROPERTY LANGUAGE C)
set_property(SOURCE Debug/mcount.S PROPERTY LANGUAGE C)
set_property(SOURCE PRX/DveMgr/pspDveManager.S PROPERTY LANGUAGE C)
set_property(SOURCE PRX/KernelButtons/imposectrl.S PROPERTY LANGUAGE C)
set_property(SOURCE PRX/MediaEngine/MediaEngine.S PROPERTY LANGUAGE C)


				#PSP Build
				add_library(plat_debug OBJECT Debug/DaedalusAssertPSP.cpp Debug/DBGConsolePSP.cpp )
				add_library(plat_dynarec OBJECT DynaRec/AssemblyWriterPSP.cpp DynaRec/AssemblyUtilsPSP.cpp DynaRec/CodeBufferManagerPSP.cpp DynaRec/CodeGeneratorPSP.cpp DynaRec/DynaRecStubs.S DynaRec/N64RegisterCachePSP.cpp)
				add_library(plat_graphics OBJECT Graphics/DrawText.cpp Graphics/GraphicsContext.cpp Graphics/NativeTexturePSP.cpp Graphics/VideoMemoryManager.cpp Graphics/intraFont/intraFont.c Graphics/intraFont/libccc.c)
				add_library(plat_audio OBJECT HLEAudio/AudioPluginPSP.cpp)
				add_library(plat_hlegraphics OBJECT HLEGraphics/Blender.cpp HLEGraphics/BlendModes.cpp HLEGraphics/ColourAdjuster.cpp HLEGraphics/ConvertVertices.S HLEGraphics/DisplayListDebugger.cpp HLEGraphics/GraphicsPluginPSP.cpp HLEGraphics/RendererPSP.cpp HLEGraphics/Combiner/BlendConstant.cpp HLEGraphics/Combiner/CombinerExpression.cpp HLEGraphics/Combiner/CombinerTree.cpp HLEGraphics/Combiner/RenderSettings.cpp HLEGraphics/TnLVFPU.S HLEGraphics/VectorClipping.S)
				add_library(plat_input OBJECT Input/InputManagerPSP.cpp)
				add_library(plat_ui OBJECT UI/AboutComponent.cpp UI/AdjustDeadzoneScreen.cpp UI/AdvancedOptionsScreen.cpp UI/CheatOptionsScreen.cpp UI/ColourPulser.cpp UI/Dialogs.cpp UI/GlobalSettingsComponent.cpp UI/MainMenuScreen.cpp UI/PauseOptionsComponent.cpp UI/PauseScreen.cpp UI/RomPreferencesScreen.cpp UI/RomSelectorComponent.cpp UI/SavestateSelectorComponent.cpp UI/SelectedRomComponent.cpp UI/SplashScreen.cpp UI/UICommand.cpp UI/UIComponent.cpp UI/UIContext.cpp UI/UIElement.cpp UI/UIScreen.cpp UI/UISetting.cpp)
				add_library(plat_utility OBJECT Utility/AtomicPrimitives.S Utility/BatteryPSP.cpp Utility/Buttons.cpp Utility/DebugMemory.cpp Utility/DisableFPUExceptions.S Utility/exception.cpp Utility/FastMemcpyPSP.cpp Utility/IOPSP.cpp Utility/JobManager.cpp Utility/ModulePSP.cpp Utility/ThreadPSP.cpp Utility/TimingPSP.cpp Utility/VolatileMemPSP.cpp)
				add_library(psp_plugins OBJECT PRX/DveMgr/pspDveManager.S PRX/KernelButtons/imposectrl.S PRX/MediaEngine/me.c PRX/MediaEngine/MediaEngine.S)

				set (GPROF_SRCS Debug/prof.c Debug/mcount.S )
