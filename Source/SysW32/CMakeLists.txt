add_subdirectory(Debug)
add_subdirectory(Utility)

install(FILES ${PROJECT_BINARY_DIR}/../Data/roms.ini DESTINATION DaedalusX64)
install(FILES ${PROJECT_BINARY_DIR}/../Data/readme.txt DESTINATION DaedalusX64)
install(FILES ${PROJECT_BINARY_DIR}/../Data/copying.txt DESTINATION DaedalusX64)

#MSVC adds some extra paths for the executable, need to cater for them (for now, we'll be able to use Clang later)
if(MSVC)
if(DEBUG)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../Debug/daedalus.exe DESTINATION DaedalusX64)
else()
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../Release/daedalus.exe DESTINATION DaedalusX64)
endif()
else()
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../daedalus.exe DESTINATION DaedalusX64)
endif()
list(APPEND daed_libs)
set(daed_libs ${daed_libs} Shlwapi.lib Dsound.lib Xinput.lib uuid.lib PARENT_SCOPE)
set(plat_main SysPosix/main.cpp PARENT_SCOPE)

file(GLOB DYNAMIC_LIBRARIES "${CMAKE_CURRENT_BINARY_DIR}/../*.dll")

install(FILES ${DYNAMIC_LIBRARIES} DESTINATION DaedalusX64)