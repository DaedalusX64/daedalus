add_library(UI_PSP OBJECT 
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
                            UIContext.cpp 
                            UIElement.cpp 
                            UIScreen.cpp 
                            UISetting.cpp
            )

            
list(APPEND daed_libs "UI_PSP")
set(daed_libs ${daed_libs} PARENT_SCOPE)
