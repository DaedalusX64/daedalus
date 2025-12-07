# Tools/cmake/mingw-g++.cmake

# Cross-compiling to Windows
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(TRIPLET x86_64-w64-mingw32)

# Custom cross root (change if you want to use /usr/x86_64-w64-mingw32)
set(CROSS_ROOT /usr/${TRIPLET})

set(CMAKE_INSTALL_PREFIX "${CROSS_ROOT}")

set(CMAKE_C_COMPILER   ${TRIPLET}-gcc-posix)
set(CMAKE_CXX_COMPILER ${TRIPLET}-g++-posix)
set(CMAKE_RC_COMPILER  ${TRIPLET}-windres)

# Where to search for includes/libs for the target
set(CMAKE_FIND_ROOT_PATH "${CROSS_ROOT}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Optional: static-ish linking
set(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
