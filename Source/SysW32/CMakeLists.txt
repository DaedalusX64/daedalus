add_subdirectory(Debug)
add_subdirectory(HLEAudio)
add_subdirectory(Interface)
add_subdirectory(Utility)

install(FILES ${PROJECT_BINARY_DIR}/../Data/roms.ini DESTINATION DaedalusX64)
install(FILES ${PROJECT_BINARY_DIR}/../Data/readme.txt DESTINATION DaedalusX64)
install(FILES ${PROJECT_BINARY_DIR}/../Data/copying.txt DESTINATION DaedalusX64)

list(APPEND daed_libs)
set(daed_libs ${daed_libs} Shlwapi.lib Dsound.lib Xinput.lib uuid.lib PARENT_SCOPE)
set(plat_main SysW32/main.cpp PARENT_SCOPE)