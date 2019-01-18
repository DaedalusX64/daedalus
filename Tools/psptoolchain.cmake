#
# CMake Toolchain for PSP
#
#
# Copyright 2019 - Wally

#Basic CMake Declarations - Required (CMAKE_SYSTEM_NAME sets cross compiling to true)
set(CMAKE_SYSTEM_NAME Generic)
set(TOOLCHAIN "${PSPDEV}")
set(CMAKE_C_COMPILER ${CC})
set(CMAKE_CXX_COMPILER ${CXX})
set(CMAKE_FIND_ROOT_PATH ${PSPDEV}/psp/sdk)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

##Get the parent PSPDev Directory to determine where the SDK is hiding
execute_process( COMMAND bash -c "psp-config --pspdev-path" OUTPUT_VARIABLE PSPDEV OUTPUT_STRIP_TRAILING_WHITESPACE)

# Set variables for PSPSDK / Bin
set(PSPSDK ${PSPDEV}/psp/sdk)
set(PSPBIN ${PSPDEV}/bin/)

#Include Directories
include_directories({${include_directories} . ${PSPSDK}/include )
ADD_DEFINITIONS("-G0")


# Set Variables for PSPSDK aplications
set(CC ${PSPBIN}psp-gcc)
set(CXX ${PSPBIN}psp-g++)
set(AS ${PSPBIN}psp-as) ##This was psp-gcc in the build.mak
set(LD ${PSPBIN}psp-gcc) ##This was psp-gcc in the build.mak
set(AR ${PSPBIN}psp-ar)
set(RANLIB ${PSPBIN}psp-ranlib)
set(STRIP1 ${PSPBIN}psp-strip) ##can't use STRIP as it's a variable in CMake
set(MKSFO ${PSPBIN}mksfo)
set(PACK_PBP ${PSPBIN}pack-pbp)
set(FIXUP ${PSPBIN}psp-fixup-imports)
set(ENC ${PSPBIN}PrxEncrypter)
