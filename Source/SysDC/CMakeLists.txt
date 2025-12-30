add_subdirectory(Graphics)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../DaedalusX64.elf DESTINATION DaedalusX64)
install(DIRECTORY ${PROJECT_BINARY_DIR}/../Data/ DESTINATION DaedalusX64)

# list(APPEND sys_libraries "imgui" "picaGL" "sqlite3")
set(sys_libraries ${sys_libraries} PARENT_SCOPE)


set(daed_libs ${daed_libs} PARENT_SCOPE)
set(sys_libraries ${sys_libraries} PARENT_SCOPE)

set(plat_main SysDC/main.cpp PARENT_SCOPE)