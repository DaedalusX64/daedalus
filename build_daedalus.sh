#!/bin/bash

## This file is the standard way of building Daedalus, you may build manually using cmake but your milage may vary on certain platforms.

PROC_NR=$(getconf _NPROCESSORS_ONLN)

function pre_prep() {
# Start Fresh, remove any existing builds
rm -r $PWD/build $PWD/DaedalusX64/EBOOT.PBP $PWD/DaedalusX64/daedalus >/dev/null 2>&1
}

function finalPrep() {
    if [[ $1 = "PSP" ]]; then
        mv "$PWD/EBOOT.PBP" ../DaedalusX64/
    else
        cp -r "$PWD/daedalus" ../DaedalusX64
        cp ../Source/SysGL/HLEGraphics/n64.psh ../DaedalusX64 
    fi
         cp -r ../Data/* ../DaedalusX64/
}

function build() {

## Build PSP extensions - Really need to make these cmake files 
if [[ $1 = "PSP" ]]; then
  make --quiet -j $PROC_NR -C "$PWD/../Source/SysPSP/PRX/DveMgr" || { exit 1; }
  make --quiet -j $PROC_NR -C "$PWD/../Source/SysPSP/PRX/ExceptionHandler" || { exit 1; }
  make --quiet -j $PROC_NR -C "$PWD/../Source/SysPSP/PRX/KernelButtons" || { exit 1; }
  make --quiet -j $PROC_NR -C "$PWD/../Source/SysPSP/PRX/MediaEngine" || { exit 1; }
fi

## Compile and install.
make --quiet -j $PROC_NR  || { exit 1; }
finalPrep
}

if [[ $1 = "DEBUG" ]] || [[ $2 = "DEBUG" ]]; then
    CMAKEDEFINES+=" -DDEBUG=1"
fi

## Main Loop

 pre_prep
 mkdir -p build && cd build

case "$1" in
    PSP)
    cmake -DCMAKE_TOOLCHAIN_FILE=$PSPDEV/psp/share/pspdev.cmake $CMAKEDEFINES ../Source
    build
    ;;
    **) ## Not Cross Compiling (macOS, Windows (Not yet), Linux)
    cmake ../Source $CMAKEDEFINES
    build
    ;;
esac

