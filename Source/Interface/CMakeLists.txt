add_library(Interface OBJECT
                            ConfigOptions.cpp
                            Cheats.cpp
                            Preferences.cpp
                            RomDB.cpp
                            SaveState.cpp                
            )

            
if(DAEDALUS_PROFILE_INTERFACE)
target_compile_options(Interface PRIVATE -pg)
endif(DAEDALUS_PROFILE_INTERFACE)

list(APPEND daed_libs "Interface")
set(daed_libs ${daed_libs} PARENT_SCOPE)
