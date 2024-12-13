add_library(Graphics OBJECT
                    ColourValue.cpp 
                    HvqmTask.cpp
                    RE2TASK.cpp
                    JpegTask.cpp
                    PngUtil.cpp 
                    TextureTransform.cpp
            )

target_include_directories(Graphics PUBLIC ${PNG_INCLUDE_DIRS})

if(DAEDALUS_PROFILE_GRAPHICS)
target_compile_options(Graphics PRIVATE -pg)
endif(DAEDALUS_PROFILE_GRAPHICS)

list(APPEND daed_libs "Graphics")
set(daed_libs ${daed_libs} PARENT_SCOPE)
