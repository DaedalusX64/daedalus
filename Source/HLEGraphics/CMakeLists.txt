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

list(APPEND daed_libs "HLEGraphics")
set(daed_libs ${daed_libs} PARENT_SCOPE)
