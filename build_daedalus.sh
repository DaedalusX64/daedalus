#!/bin/bash

## TODO:
# Enable use of nproc for processor detection so builds according to number of processors in machine (Darwin will need sysctl -n hw.logicalcpu)

## Determine the maximum number of processes that Make can work with.
PROC_NR=$(getconf _NPROCESSORS_ONLN)

PLATFORM=$1"build"
echo $PLATFORM
function pre_prep(){

    if [ -d $PWD/$PLATFORM ]; then

        echo "Removing previous build attempt"
        rm -r "$PWD/$PLATFORM"
    fi   

    if [ -d $PWD/DaedalusX64 ]; then
        rm -r $PWD/DaedalusX64/EBOOT.PBP
    else
        mkdir $PWD/DaedalusX64
        mkdir ../DaedalusX64/SaveStates
        mkdir ../DaedalusX64/SaveGames
        mkdir ../DaedalusX64/Roms
    fi
        mkdir "$PWD/$PLATFORM"
}

function finalPrep() {

    if [ ! -d ../DaedalusX64 ]; then
        mkdir ../DaedalusX64/SaveStates ../DaedalusX64/SaveGames ../DaedalusX64/Roms
    fi

    if [ -f "$PWD/EBOOT.PBP" ]; then
        mv "$PWD/EBOOT.PBP" ../DaedalusX64/
        cp -r ../Data/* ../DaedalusX64/
    else
        cp -r "$PWD/daedalus" ../DaedalusX64
        cp -r ../Data/* ../DaedalusX64/
        cp ../Source/SysGL/HLEGraphics/n64.psh ../DaedalusX64
    fi
}

function build() {

## Build PSP extensions - Really need to make these cmake files 
if [[ $PLATFORM = "PSPbuild" ]]; then
  make --quiet -j $PROC_NR -C "$PWD/../Source/SysPSP/PRX/DveMgr" || { exit 1; }
  make --quiet -j $PROC_NR -C "$PWD/../Source/SysPSP/PRX/ExceptionHandler" || { exit 1; }
  make --quiet -j $PROC_NR -C "$PWD/../Source/SysPSP/PRX/KernelButtons" || { exit 1; }
  make --quiet -j $PROC_NR -C "$PWD/../Source/SysPSP/PRX/MediaEngine" || { exit 1; }
fi

## Compile and install.
make --quiet -j $PROC_NR            || { exit 1; }
finalPrep
}

#Add Defines for CMake to not mess up the main loop
if [[ $1 = "PSP" ]]; then

    CMAKEDEFINES+="-DCMAKE_TOOLCHAIN_FILE=$PSPDEV/psp/share/pspdev.cmake"
fi
 
CMAKEDEFINES+=" -D$1=1"
if [[ $2 = "DEBUG" ]]; then
    CMAKEDEFINES+=" -D$2=1"
fi

## Main()
## Do our OS checking here
case "$1" in
    PSP | LINUX | MAC | WIN)
    pre_prep
    cd $PLATFORM
    cmake $CMAKEDEFINES ../Source
    build
    ;;
    **)
    echo "Build Options: PSP / LINUX / MAC / WIN"
    echo "Building debug or profile builds requires DEBUG or PROFILE after build option"
    ;;
    esac
    
if [[ $1 = "LINUX" ]]; then
    cp Source/SysGL/HLEGraphics/n64.psh LINUXbuild
fi
