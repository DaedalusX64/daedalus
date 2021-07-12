
set(DEVKITPRO $ENV{DEVKITPRO})

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "clang --target arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "clang --target arm-none-eabi-g++")
set(CMAKE_AR "llvm-ar" CACHE STRING "")
set(CMAKE_ASM_COMPILER "clang --target arm-none-eabi-gcc")
set(CMAKE_RANLIB "llvm-ranlib CACHE STRING "")
set(CMAKE_ASM_COMPILER "clang --target arm-none-eabi-gcc")

set(ARCH "-march=armv6k -mtune=mpcore -mfloat-abi=hard -mfpu=vfp -mtp=soft -D_3DS")
set(CMAKE_C_FLAGS "${ARCH} -Wall -mword-relocations -O3 -fomit-frame-pointer -ffunction-sections -fdata-sections" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -std=gnu++14" CACHE STRING "C++ flags")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_FIND_ROOT_PATH ${DEVKITPRO}/devkitARM ${DEVKITPRO}/libctru ${DEVKITARM}/portlibs/3ds)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Shared libs not available")

link_directories(${DEVKITPRO}/libcrtu/lib ${DEVKITPRO}/portlibs/3ds/lib)