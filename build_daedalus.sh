#!/bin/bash

PROC_NR=$(getconf _NPROCESSORS_ONLN)

## This file is the standard way of building Daedalus, you may build manually using cmake but your milage may vary on certain platforms.

#Clear last build
rm -r build 

 function psp_plugins() {
   make --quiet -j $PROC_NR -C "$PWD/Source/SysPSP/PRX/DveMgr" || { exit 1; }
   make --quiet -j $PROC_NR -C "$PWD/Source/SysPSP/PRX/ExceptionHandler" || { exit 1; }
   make --quiet -j $PROC_NR -C "$PWD/Source/SysPSP/PRX/KernelButtons" || { exit 1; }
   make --quiet -j $PROC_NR -C "$PWD/Source/SysPSP/PRX/MediaEngine" || { exit 1; }
 }


if [[ $1 = "DEBUG" ]] || [[ $2 = "DEBUG" ]]; then
    CMAKEDEFINES+=" -DDEBUG=1"
fi


# Add any custom console toolchains here
case "$1" in
    PSP)
    psp_plugins
    TOOLCHAIN+="-DCMAKE_TOOLCHAIN_FILE=$PSPDEV/psp/share/pspdev.cmake"  
    ;;
esac

    cmake $TOOLCHAIN $CMAKEDEFINES -S . -B build 
    cmake --build build -j${PROC_NR}
    cmake --install build --prefix $PWD
