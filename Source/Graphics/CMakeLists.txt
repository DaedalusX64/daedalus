add_library(Graphics OBJECT
                    ColourValue.cpp 
                    HvqmTask.cpp
                    JpegTask.cpp
                    PngUtil.cpp 
                    TextureTransform.cpp
            )

target_include_directories(Graphics PUBLIC ${PNG_INCLUDE_DIRS})

list(APPEND daed_libs "Graphics")
set(daed_libs ${daed_libs} PARENT_SCOPE)
