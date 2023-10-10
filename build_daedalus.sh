#!/bin/bash

PROC_NR=$(getconf _NPROCESSORS_ONLN)

## This file is the standard way of building Daedalus, you may build manually using cmake but your milage may vary on certain platforms.

#Clear last build
rm -r build 

 function psp_plugins() {
   make --quiet -j $PROC_NR -C "$PWD/Source/SysPSP/PRX/DveMgr" || { exit 1; }
   make --quiet -j $PROC_NR -C "$PWD/Source/SysPSP/PRX/ExceptionHandler" || { exit 1; }
   make --quiet -j $PROC_NR -C "$PWD/Source/SysPSP/PRX/MediaEngine" || { exit 1; }
 }


if [[ $1 = "DEBUG" ]] || [[ $2 = "DEBUG" ]]; then
    CMAKEDEFINES+=" -DDEBUG=1"
fi


# Add any custom console toolchains
case "$1" in
    PSP)
    psp_plugins
    CMAKE=psp-cmake
    ;;
    CTR)
    CMAKE="cmake -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/3DS.cmake""
    CMAKEDEFINES+="-DCTR=1"
    ;;
    *)
    CMAKE=cmake
    CMAKEDEFINES=""
    ;;
esac
    # Use the custom define to do initial build then parse cmake after
    $CMAKE $CMAKEDEFINES -S . -B build 
    cmake --build build -j${PROC_NR}
    cmake --install build --prefix $PWD
