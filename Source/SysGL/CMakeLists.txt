				#SysGL
				include_directories(/usr/local/include) 
				find_library(OPENGL_LIBRARY OpenGL)
				find_package(SDL2)
				if(APPLE)
					find_library(COCOA_LIBRARY Cocoa )
					find_library(IOKIT_LIBRARY IOKit )
					find_library(COREVID_LIBRARY CoreVideo )
					find_library(AUDIOTOOLBOX_LIBRARY AudioToolBox)
				elseif(UNIX)
					find_package(GLEW)
				endif(APPLE)


				include_directories(${OPENGL_INCLUDE_DIR})
				link_libraries( ${OPENGL_LIBRARY} ${COCOA_LIBRARY} -lGLEW ${SDL2_LIBRARIES} ${IOKIT_LIBRARY} ${COREVID_LIBRARY} ${AUDIOTOOLBOX_LIBRARY} )

				add_library(plat_graphics OBJECT Graphics/GraphicsContextGL.cpp Graphics/NativeTextureGL.cpp)
				add_library(plat_hlegraphics OBJECT HLEGraphics/GraphicsPluginGL.cpp HLEGraphics/RendererGL.cpp)
				add_library(plat_input OBJECT Input/InputManagerGL.cpp)
				add_library(plat_ui OBJECT Interface/UI.cpp)
