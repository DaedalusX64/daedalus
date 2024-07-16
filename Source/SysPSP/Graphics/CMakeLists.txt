add_library(Graphics_PSP OBJECT 
                                DrawTextPSP.cpp 
                                GraphicsContextPSP.cpp 
                                NativeTexturePSP.cpp 
                                VideoMemoryManager.cpp
                                UIContextPSP.cpp
            )

            
list(APPEND daed_libs "Graphics_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
