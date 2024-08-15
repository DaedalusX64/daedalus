add_library(Interface OBJECT
                            ConfigOptions.cpp
                            Cheats.cpp
                            Preferences.cpp
                            RomDB.cpp
                            SaveState.cpp                
            )

            
list(APPEND daed_libs "Interface")
set(daed_libs ${daed_libs} PARENT_SCOPE)
