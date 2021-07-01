				#SysGL
				add_library(plat_graphics Graphics/GraphicsContextGL.cpp Graphics/NativeTextureGL.cpp)
				add_library(plat_hlegraphics HLEGraphics/GraphicsPluginGL.cpp HLEGraphics/RendererGL.cpp)
				add_library(plat_input Input/InputManagerGL.cpp)
				add_library(plat_ui Interface/UI.cpp)

				add_definitions(-DDAEDALUS_GL)