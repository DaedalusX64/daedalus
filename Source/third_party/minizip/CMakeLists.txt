if(CMAKE_SYSTEM_NAME STREQUAL Windows)
set(WIN32 iowin32.c)
add_definitions(-DWIN32)
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin OR Linux)
add_definitions(-Dunix)
endif(CMAKE_SYSTEM_NAME STREQUAL Windows)



add_library(minizip ioapi.c miniunz.c mztools.c unzip.c zip.c ${WIN32})

