add_library(Plugins_PSP OBJECT 
                                KernelButtons/imposectrl.S
                                DveMgr/pspDveManager.S 
                                MediaEngine/me.c 
                                MediaEngine/MediaEngine.S
            )   

            
            list(APPEND daed_libs "Plugins_PSP")
            set(daed_libs ${daed_libs} PARENT_SCOPE)
            