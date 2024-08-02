add_subdirectory(Debug)
add_subdirectory(Graphics)
add_subdirectory(HLEGraphics)
add_subdirectory(Input)
add_subdirectory(PRX)
add_subdirectory(Utility)

# Create and install Plugin files

install(DIRECTORY DESTINATION DaedalusX64/Plugins) 
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/PRX/MediaEngine/mediaengine.prx DESTINATION DaedalusX64/Plugins)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/PRX/DveMgr/dvemgr.prx DESTINATION DaedalusX64/Plugins)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}//PRX/ExceptionHandler/exception.prx DESTINATION DaedalusX64/Plugins)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../EBOOT.PBP DESTINATION DaedalusX64)

#Copy the PRX and plain binary to the DaedalusX64 Directory for testing
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../daedalus.prx DESTINATION DaedalusX64)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../daedalus DESTINATION DaedalusX64/daedalus_psp) #Named daedalus_psp to avoid having to overwrite other binaries
endif()

list(APPEND daed_libs)
set(daed_libs ${daed_libs} PARENT_SCOPE)

if(DAEDALUS_SDL)
list(APPEND sys_libraries pspkubridge pspfpu pspdebug SDL2_ttf freetype harfbuzz bz2)
else()
list(APPEND sys_libraries intrafont pspkubridge pspfpu pspdebug pspgu pspge pspaudio pspaudiolib psppower pspctrl pspdisplay)
endif()
 set(sys_libraries ${sys_libraries} PARENT_SCOPE)

set(plat_main SysPSP/main.cpp PARENT_SCOPE)
