
add_library(UI_CTR OBJECT
            InGameMenu.cpp
            RomSelector.cpp
            UserInterface.cpp
            )
            if(DAEDALUS_PROFILE_UI_CTR)
            target_compile_options(UI_CTR PRIVATE -pg)
       endif(DAEDALUS_PROFILE_UI_CTR)
                                   
            
list(APPEND daed_libs "UI_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)
