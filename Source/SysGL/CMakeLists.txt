
add_subdirectory(Graphics)
add_subdirectory(HLEGraphics)

## These two should probably be in SysSDL2
add_subdirectory(Input) 
add_subdirectory(Interface)


    if (UNIX AND NOT APPLE)
    list(APPEND sys_libraries "GL")
endif()
    list(APPEND sys_libraries "GLEW")


set(sys_libraries ${sys_libraries} PARENT_SCOPE)

list(APPEND daed_libs)
set(daed_libs ${daed_libs} PARENT_SCOPE)
