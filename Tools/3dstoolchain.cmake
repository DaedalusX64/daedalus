
set(DEVKITPRO $ENV{DEVKITPRO})

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR "arm6k")
set(CMAKE_C_COMPILER "${DEVKITPRO}/devkitARM/bin/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "${DEVKITPRO}/devkitARM/bin/arm-none-eabi-g++")
set(CMAKE_AR "${DEVKITPRO}/devkitARM/bin/arm-none-eabi-gcc-ar" CACHE STRING "")
set(CMAKE_RANLIB "${DEVKITPRO}/devkitARM/bin/arm-none-eabi-gcc-ranlib" CACHE STRING "")
set(CMAKE_ASM_COMPILER "${DEVKITPRO}/devkitARM/bin/arm-none-eabi-gcc")

set(ARCH "-march=armv6k -mtune=mpcore -mfloat-abi=hard -mfpu=vfp -mtp=soft -D__3DS__")
set(CMAKE_C_FLAGS "${ARCH} -Wall -mword-relocations -O3 -fomit-frame-pointer -ffunction-sections -fdata-sections" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -std=gnu++14" CACHE STRING "C++ flags")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_FIND_ROOT_PATH ${DEVKITPRO}/devkitARM ${DEVKITPRO}/libctru ${DEVKITARM}/portlibs/3ds)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Shared libs not available")

include_directories(${DEVKITPRO}/libctru/include)
link_directories(${DEVKITPRO}/libctru/lib ${DEVKITPRO}/portlibs/3ds/lib)
