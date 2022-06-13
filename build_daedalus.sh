#!/bin/bash

PROC_NR=$(getconf _NPROCESSORS_ONLN)

## This file is the standard way of building Daedalus, you may build manually using cmake but your milage may vary on certain platforms.

function pre_prep() {
# Start Fresh, remove any existing builds
 rm -r $PWD/build $PWD/DaedalusX64/EBOOT.PBP $PWD/DaedalusX64/daedalus >/dev/null 2>&1
}


 function psp_plugins() {
   make --quiet -j $PROC_NR -C "$PWD/Source/SysPSP/PRX/DveMgr" || { exit 1; }
   make --quiet -j $PROC_NR -C "$PWD/Source/SysPSP/PRX/ExceptionHandler" || { exit 1; }
   make --quiet -j $PROC_NR -C "$PWD/Source/SysPSP/PRX/KernelButtons" || { exit 1; }
   make --quiet -j $PROC_NR -C "$PWD/Source/SysPSP/PRX/MediaEngine" || { exit 1; }
 }

function build() {

## Compile and install.
make --quiet -j $PROC_NR  || { exit 1; }
}

if [[ $1 = "DEBUG" ]] || [[ $2 = "DEBUG" ]]; then
    CMAKEDEFINES+=" -DDEBUG=1"
fi

## Main Loop

 pre_prep

case "$1" in
    PSP)
    psp_plugins
    cmake -DCMAKE_TOOLCHAIN_FILE=$PSPDEV/psp/share/pspdev.cmake $CMAKEDEFINES -S . -B build 
    cmake --build build -j${PROC_NR}
    cmake --install build --prefix .
    # cmake --install build
    ;;
    **) ## Not Cross Compiling (macOS, Windows (Not yet), Linux)
    cmake ../Source $CMAKEDEFINES
    build
    ;;
esac

