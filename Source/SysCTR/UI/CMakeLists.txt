
add_library(UI_CTR OBJECT
            InGameMenu.cpp
            RomSelector.cpp
            UserInterface.cpp
            )

            
list(APPEND daed_libs "UI_CTR")
set(daed_libs ${daed_libs} PARENT_SCOPE)
