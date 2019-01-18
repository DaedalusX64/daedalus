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
#set(CMAKE_SYSROOT {PSPDEV})
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


#Lib Directories & Libs


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





#[[
Logic for PBP Packing
Not really worried about making PRXS right now
Logic
pack-pbp <output.pbp> <param.sfo> <icon0.png> <icon1.pmf> <pic0.png> <pic1.png> <snd0.at3> <data.psp> <data.psar>

#Set the variables to default Daedalus for now - NULL is 0

set(EBOOT_SFO "NULL")
set(EBOOT_ICON "{PROJECT_SOURCE_DIR}/Resources/icon0.png")
set(EBOOT_ICON1 "NULL")
set(EBOOT_PIC0 "{PROJECT_SOURCE_DIR}/Resources/pic1.png")
set(EBOOT_PIC1 "NULL")
set(EBOOT_SND0 "NULL")
set(EBOOT_DATAPSP "NULL")
set(EBOOT_DATAPSAR "NULL")

#Build the PBP File
execute_process( COMMAND bash -c "pbp-pack EBOOT.PBP {EBOOT_SFO} {EBOOT_ICON} {EBOOT_ICON1} {EBOOT_PIC0} {EBOOT_PIC1} {EBOOT_SND0} {EBOOT_DATAPSP} {EBOOT_DATAPSAR}")
]]
