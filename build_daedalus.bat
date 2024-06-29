REM This requires several packages to be installed before it will work
cmake -G Ninja -B build -DCMAKE_TOOLCHAIN_FILE="C:\Utilities\vcpkg\scripts\buildsystems\vcpkg.cmake" -DCMAKE_INSTALL_PREFIX="."
ninja -C build/
ninja -C build install