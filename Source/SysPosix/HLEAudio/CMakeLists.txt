if(UNIX AND NOT APPLE)
add_library(Posix_Audio OBJECT AudioPluginSDL.cpp)
elseif( APPLE)
add_library(Posix_Audio OBJECT AudioPluginOSX.cpp)
else() ## No Audio available
add_library(Posix_Audio OBJECT AudioPluginNone.cpp)
endif()
