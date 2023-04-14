add_library(Interface_Win OBJECT 
                                FileNameHandler.cpp
            )

list(APPEND daed_libs Interface_Win)
set(daed_libs ${daed_libs} PARENT_SCOPE)

