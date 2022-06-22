add_subdirectory(Debug)
add_subdirectory(DynaRec)
add_subdirectory(HLEAudio)
add_subdirectory(HLEGraphics)
add_subdirectory(Utility)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../daedalus DESTINATION DaedalusX64
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

install(FILES ${PROJECT_SOURCE_DIR}/Source/SysGL/HLEGraphics/n64.psh DESTINATION DaedalusX64)
install(FILES ${PROJECT_BINARY_DIR}/../Data/roms.ini DESTINATION DaedalusX64)
install(FILES ${PROJECT_BINARY_DIR}/../Data/readme.txt DESTINATION DaedalusX64)
install(FILES ${PROJECT_BINARY_DIR}/../Data/copying.txt DESTINATION DaedalusX64)