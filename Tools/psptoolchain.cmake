set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_CROSSCOMPILING TRUE)

set(BUILD_SHARED_LIBS FALSE)
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# find executables
foreach(i "psp-config" "mksfo" "pack-pbp" "psp-fixup-imports" "psp-strip" "psp-prxgen" "psp-build-exports" "psp-gcc" "psp-g++")
  string(REGEX REPLACE "-" "_" variable ${i})
  string(TOUPPER ${variable} variable)

  find_program(${variable} ${i})
  if(${${variable}} MATCHES "-NOTFOUND")
	message(FATAL_ERROR "${i} not found")
  else()
	message(STATUS "${i} found")
  endif()
endforeach()

# set compiler

set( CMAKE_C_COMPILER "psp-gcc")
set( CMAKE_CXX_COMPILER "psp-g++")

# set find root path
set(CMAKE_FIND_ROOT_PATH "")
FOREACH(i "--pspsdk-path" "--psp-prefix" "--pspdev-path")
  EXECUTE_PROCESS(
	COMMAND ${PSP_CONFIG} ${i}
	OUTPUT_VARIABLE output
	OUTPUT_STRIP_TRAILING_WHITESPACE)

  LIST(APPEND CMAKE_FIND_ROOT_PATH "${output}")
  INCLUDE_DIRECTORIES(SYSTEM "${output}/include")
  LINK_DIRECTORIES("${output}/lib")
  list(APPEND CMAKE_EXE_LINKER_FLAGS "-L${output}/lib")
ENDFOREACH()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# set install path
EXECUTE_PROCESS(
  COMMAND ${PSP_CONFIG} "--psp-prefix"
  OUTPUT_VARIABLE CMAKE_INSTALL_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# large memory
if(PSP_LARGE_MEMORY)
  find_program(MKSFO "mksfoex")
  if(NOT MKSFO)
	message(FATAL_ERROR "mksfoex not found")
  else()
	message(STATUS "mksfoex found")
  endif()
  set(MKSFO "${MKSFO} -d MEMSIZE=1")
endif()

# firmware vesion
if(NOT PSP_FW_VERSION)
  set(PSP_FW_VERSION 661) # default
endif()
add_definitions("-D _PSP_FW_VERSION=${PSP_FW_VERSION}")

# pspsdk path
EXECUTE_PROCESS(
  COMMAND ${PSP_CONFIG} "--pspsdk-path"
  OUTPUT_VARIABLE PSPSDK_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# reduce link error
ADD_DEFINITIONS("-G0")

# linker flags
# libc
if(USE_KERNEL_LIBC)
  INCLUDE_DIRECTORIES(${PSPSDK_PATH}/include/libc)
elseif(USE_PSPSDK_LIBC)
  INCLUDE_DIRECTORIES(${PSPSDK_PATH}/include/libc)
  set(PSP_LIBC -lpsplibc)
else()
  set(PSP_LIBC -lc)
endif()
# pspsdk libs
if(USE_KERNEL_LIBS)
  set(PSPSDK_LIBRARIES -lpspdebug -lpspctrl_driver -lpspctrl_driver -lpspsdk)
  set(PSPSDK_LIBRARIES_2 -lpspkernel)
elseif(USE_USER_LIBS)
  set(PSPSDK_LIBRARIES -lpspdebug -lpspctrl -lpspge -lpspctrl -lpspsdk)
  set(PSPSDK_LIBRARIES_2
	-lpspnet -lpspnet_inet -lpspnet_apctl -lpspnet_resolver
	-lpsputility -lpspuser -lpspgu)
else()
  set(PSPSDK_LIBRARIES -lpspdebug -lpspctrl -lpspge -lpspctrl -lpspsdk)
  set(PSPSDK_LIBRARIES_2
	-lpspnet -lpspnet_inet -lpspnet_apctl -lpspnet_resolver
	-lpsputility -lpspuser -lpspkernel)
endif()
list(APPEND CMAKE_EXE_LINKER_FLAGS
  -lpthread-psp -flto ${PSPSDK_LIBRARIES} ${PSP_LIBC} ${PSPSDK_LIBRARIES_2})

# prx
if(BUILD_PRX)
  list(APPEND CMAKE_EXE_LINKER_FLAGS
	"-specs=${PSPSDK_PATH}/lib/prxspecs"
	"-Wl,-q,-T${PSPSDK_PATH}/lib/linkfile.prx")
  if(PRX_EXPORTS)
	string(REGEX REPLACE ".exp$" ".o" EXPORT_OBJ ${PRX_EXPORTS})
  else()
	set(EXPORT_OBJ ${PSPSDK_PATH}/lib/prxexports.o)
  endif()
endif()

# set ldflags
string(REPLACE ";" " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} CACHE STRING "")

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
