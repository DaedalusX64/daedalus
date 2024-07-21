#Ideally Daedalus SDL would trigger the font off.
if(INTRAFONT)
set(DRAW_TEXT DrawTextPSP.cpp) 
else()
set(DRAW_TEXT ${PROJECT_SOURCE_DIR}/Source/SysGL/Graphics/DrawTextSDL.cpp)
endif()

add_library(Graphics_PSP OBJECT 

                                 GraphicsContextPSP.cpp 
                                NativeTexturePSP.cpp 
                                VideoMemoryManager.cpp
                                UIContextPSP.cpp
                                ${DRAW_TEXT}
            )

            
list(APPEND daed_libs "Graphics_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
