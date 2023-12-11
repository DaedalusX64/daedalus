add_library(Graphics_PSP OBJECT 
                                DrawTextPSP.cpp 
                                GraphicsContext.cpp 
                                NativeTexturePSP.cpp 
                                VideoMemoryManager.cpp
                                UIContext.cpp
            )

            
list(APPEND daed_libs "Graphics_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
