add_subdirectory(Debug)
add_subdirectory(DynaRec)
add_subdirectory(HLEAudio)
add_subdirectory(HLEGraphics)
add_subdirectory(Utility)

install(FILES ${PROJECT_SOURCE_DIR}/daedalus DESTINATION DaedalusX64)
install(FILES ${PROJECT_SOURCE_DIR}/SysGL/HLEGraphics/n64.psh DESTINATION DaedalusX64)