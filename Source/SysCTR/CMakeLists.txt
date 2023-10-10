add_subdirectory(Debug)
add_subdirectory(Graphics)
add_subdirectory(HLEAudio)
add_subdirectory(HLEGraphics)
add_subdirectory(Input)
add_subdirectory(UI)
add_subdirectory(Utility)


list(APPEND sys_libraries "imgui" "picaGL")
set(sys_libraries ${sys_libraries} PARENT_SCOPE)


set(daed_libs ${daed_libs} PARENT_SCOPE)
set(sys_libraries ${sys_libraries} PARENT_SCOPE)
set(plat_main SysCTR/main.cpp PARENT_SCOPE)