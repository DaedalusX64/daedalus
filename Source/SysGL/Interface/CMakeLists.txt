add_library(Interface_GL OBJECT UI.cpp  
            )

            
list(APPEND daed_libs Interface_GL)
set(daed_libs ${daed_libs} PARENT_SCOPE)
            