add_subdirectory(Debug)
add_subdirectory(DynaRec)
add_subdirectory(HLEAudio)
add_subdirectory(HLEGraphics)
add_subdirectory(Utility)

install(FILES ${PROJECT_BINARY_DIR}/Source/daedalus DESTINATION DaedalusX64)
install(FILES ${PROJECT_SOURCE_DIR}/Source/SysGL/HLEGraphics/n64.psh DESTINATION DaedalusX64)