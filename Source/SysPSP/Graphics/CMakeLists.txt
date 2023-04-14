add_library(Graphics_PSP OBJECT 
                                DrawText.cpp 
                                GraphicsContext.cpp 
                                NativeTexturePSP.cpp 
                                VideoMemoryManager.cpp
            )

            
list(APPEND daed_libs "Graphics_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
