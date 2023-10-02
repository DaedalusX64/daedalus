if(UNIX AND NOT APPLE)
add_library(Audio_POSIX OBJECT AudioPluginSDL.cpp)
elseif( APPLE)
add_library(Audio_POSIX OBJECT AudioPluginOSX.cpp)
else() ## No Audio available
add_library(Audio_POSIX OBJECT AudioPluginSDL.cpp)
endif()


list(APPEND daed_libs Audio_POSIX)
set(daed_libs ${daed_libs} PARENT_SCOPE)

