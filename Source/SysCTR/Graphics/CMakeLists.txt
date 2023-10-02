add_library(Graphics_Plat OBJECT GraphicsContextCTR.cpp NativeTextureCTR.cpp)

add_library(Graphics_CTR OBJECT 
            GraphicsContextCTR.cpp
            NativeTextureCTR.cpp                        
)

            
list(APPEND daed_libs "Graphics_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)
