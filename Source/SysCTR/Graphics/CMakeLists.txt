add_library(Graphics_CTR OBJECT 
            GraphicsContextCTR.cpp
            NativeTextureCTR.cpp                        
)

if(DAEDALUS_PROFILE_GRAPHICS_CTR)
    target_compile_options(Graphics_CTR PRIVATE -pg)
endif(DAEDALUS_PROFILE_GRAPHICS_CTR)
            
            
list(APPEND daed_libs "Graphics_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)
