add_library(Graphics OBJECT
                    ColourValue.cpp 
                    PngUtil.cpp 
                    TextureTransform.cpp
            )

list(APPEND daed_libs "Graphics")
set(daed_libs ${daed_libs} PARENT_SCOPE)
