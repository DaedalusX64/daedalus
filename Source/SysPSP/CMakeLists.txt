add_subdirectory(Debug)
add_subdirectory(DynaRec)
add_subdirectory(Graphics)
add_subdirectory(HLEAudio)
add_subdirectory(HLEGraphics)
add_subdirectory(Input)
add_subdirectory(PRX)
add_subdirectory(UI)
add_subdirectory(Utility)


## Build EBoot 

create_pbp_file(
    TARGET daedalus
    ICON_PATH ${PROJECT_SOURCE_DIR}/Source/SysPSP/Resources/eboot_icons/icon0.png
    BACKGROUND_PATH ${PROJECT_SOURCE_DIR}/Source/SysPSP/Resources/eboot_icons/pic1.png
    PREVIEW_PATH NULL
    TITLE DaedalusX64
    BUILD_PRX
            )
# Create and install Plugin files

install(DIRECTORY DESTINATION DaedalusX64/Plugins) 
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/PRX/MediaEngine/mediaengine.prx DESTINATION DaedalusX64/Plugins)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/PRX/DveMgr/dvemgr.prx DESTINATION DaedalusX64/Plugins)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}//PRX/ExceptionHandler/exception.prx DESTINATION DaedalusX64/Plugins)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../EBOOT.PBP DESTINATION DaedalusX64)

#PSP Resources
install(DIRECTORY ${PROJECT_BINARY_DIR}/../Data/ DESTINATION DaedalusX64)
