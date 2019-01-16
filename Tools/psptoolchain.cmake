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
link_directories(${link_directories} . ${PSPSDK}/lib)
link_libraries(-lc -lpspuser -lpspkernel -lpspdebug -lpspdisplay -lpspge -lpspctrl -lpspsdk -lpsputility)

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

# target name
if(NOT PSP_TARGET)
  SET(PSP_TARGET ${PROJECT_NAME})
endif()

# EBOOT
if(NOT PSP_EBOOT_TITLE)
  set(PSP_EBOOT_TITLE ${PSP_TARGET})
endif()
if(NOT PSP_EBOOT_SFO)
  set(PSP_EBOOT_SFO "PARAM.SFO")
endif()
if(NOT PSP_EBOOT_ICON)
  set(PSP_EBOOT_ICON "NULL")
endif()
if(NOT PSP_EBOOT_ICON1)
  set(PSP_EBOOT_ICON1 "NULL")
endif()
if(NOT PSP_EBOOT_UNKPNG)
  set(PSP_EBOOT_UNKPNG "NULL")
endif()
if(NOT PSP_EBOOT_PIC1)
  set(PSP_EBOOT_PIC1 "NULL")
endif()
if(NOT PSP_EBOOT_SND0)
  set(PSP_EBOOT_SND0 "NULL")
endif()
if(NOT PSP_EBOOT_PSAR)
  set(PSP_EBOOT_PSAR "NULL")
endif()
if(NOT PSP_EBOOT)
  set(PSP_EBOOT "EBOOT.PBP")
endif()

add_custom_command(OUTPUT ${PSP_EBOOT_SFO}
  COMMAND ${MKSFO} "'${PSP_EBOOT_TITLE}'" ${PSP_EBOOT_SFO})

if(BUILD_PRX)
  add_custom_command(OUTPUT ${PSP_TARGET}.prx
	COMMAND ${PSP_PRXGEN} ${PSP_TARGET}${CMAKE_EXECUTABLE_SUFFIX} ${PSP_TARGET}.prx
	DEPENDS ${PSP_TARGET})
  add_custom_command(OUTPUT ${PSP_EBOOT}
	COMMAND ${PACK_PBP} ${PSP_EBOOT} ${PSP_EBOOT_SFO} ${PSP_EBOOT_ICON}
	  ${PSP_EBOOT_ICON1} ${PSP_EBOOT_UNKPNG} ${PSP_EBOOT_PIC1}
	  ${PSP_EBOOT_SND0} ${PSP_TARGET}.prx ${PSP_EBOOT_PSAR}
	DEPENDS ${PSP_EBOOT_SFO} ${PSP_TARGET}.prx)
  foreach(i ${PRX_EXPORTS})
	string(REGEX REPLACE ".exp$" ".c" src ${i})
	add_custom_command(OUTPUT ${src}
	  COMMAND ${PSP_BUILD_EXPORTS} -b ${i} > ${src}
	  DEPENDS ${i})
  endforeach()
  string(REGEX REPLACE ".exp" ".c" prx_srcs ${PRX_EXPORTS})
  add_library(prx_exports ${prx_srcs})
  target_link_libraries(${PSP_TARGET} prx_exports)
elseif(BUILD_EBOOT)
  add_custom_command(OUTPUT ${PSP_EBOOT}
	COMMAND ${PSP_STRIP} ${PSP_TARGET}${CMAKE_EXECUTABLE_SUFFIX}
	  -o ${PSP_TARGET}_strip${CMAKE_EXECUTABLE_SUFFIX}
	COMMAND ${PACK_PBP} ${PSP_EBOOT} ${PSP_EBOOT_SFO} ${PSP_EBOOT_ICON}
	  $(PSP_EBOOT_ICON1) ${PSP_EBOOT_UNKPNG} $(PSP_EBOOT_PIC1)
	$(PSP_EBOOT_SND0)  ${TARGET}_strip${CMAKE_EXECUTABLE_SUFFIX} ${PSP_EBOOT_PSAR}
	COMMAND rm -f ${TARGET}_strip${CMAKE_EXECUTABLE_SUFFIX}
	DEPENDS ${PSP_EBOOT_SFO} ${PSP_TARGET})
endif()
