add_library(UI OBJECT 
                            AboutComponent.cpp 
                            AdjustDeadzoneScreen.cpp 
                            AdvancedOptionsScreen.cpp 
                            CheatOptionsScreen.cpp 
                            ColourPulser.cpp 
                            Dialogs.cpp 
                            GlobalSettingsComponent.cpp 
                            MainMenuScreen.cpp 
                            PauseOptionsComponent.cpp 
                            PauseScreen.cpp 
                            RomPreferencesScreen.cpp 
                            RomSelectorComponent.cpp 
                            SavestateSelectorComponent.cpp 
                            SelectedRomComponent.cpp 
                            SplashScreen.cpp 
                            UICommand.cpp 
                            UIComponent.cpp 
                            UIElement.cpp 
                            UIScreen.cpp 
                            UISetting.cpp
                            DrawTextUtilities.cpp
            )

if(DAEDALUS_GL)
target_include_directories(UI PUBLIC ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})
endif(DAEDALUS_GL)

list(APPEND daed_libs "UI")
set(daed_libs ${daed_libs} PARENT_SCOPE)
