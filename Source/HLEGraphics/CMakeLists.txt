add_library(HLEGraphics OBJECT 
                            BaseRenderer.cpp 
                            CachedTexture.cpp 
                            ConvertImage.cpp 
                            ConvertTile.cpp 
                            DLDebug.cpp 
                            DLParser.cpp 
                            Microcode.cpp 
                            RDP.cpp 
                            RDPStateManager.cpp 
                            TextureCache.cpp 
                            TextureCacheWebDebug.cpp 
                            TextureInfo.cpp 
                            TMEM.cpp 
                            uCodes/Ucode.cpp
            )

if(DAEDALUS_GL)
    message(STATUS "Building HLEGraphics with OpenGL support: ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS}")
    target_include_directories(HLEGraphics PUBLIC ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})
endif(DAEDALUS_GL)

list(APPEND daed_libs "HLEGraphics")
set(daed_libs ${daed_libs} PARENT_SCOPE)
