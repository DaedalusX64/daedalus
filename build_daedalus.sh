#!/bin/bash


function usage() {
echo "Usage ./build_daedalus.sh BUILD_TYPE"
echo "Build Types:"
echo "PSP Release = PSP_RELEASE"
echo "PSP Debug = PSP_DEBUG"
echo "Linux Release = LINUX_RELEASE"
echo "Mac Release = MAC_RELEASE"
exit
}

function cleanup(){
  rm -r "$PWD/daedbuild"
  mkdir "$PWD/daedbuild"
}

function finalPrep() {
  mkdir ../DaedalusX64
  mkdir ../DaedalusX64/SaveStates
  mkdir ../DaedalusX64/SaveGames
  mkdir ../DaedalusX64/Roms
    if [ "$1" = PSP_RELEASE ] || [ "$1" = PSP_DEBUG ]; then
      cp -R ../Data/PSP/* ../DaedalusX64
    else
      cp -R ../Data/PC/* ../DaedalusX64
      cp ../Source/SysGL/HLEGraphics/n64.psh ../DaedalusX64
    fi
}

function buildPSP() {

  make -C "$PWD/../Source/SysPSP/DveMgr"
  make -C "$PWD/../Source/SysPSP/ExceptionHandler/prx"
  make -C "$PWD/../Source/SysPSP/KernelButtonsPrx"
  make -C "$PWD/../Source/SysPSP/MediaEnginePRX"

  make
  finalPrep
  #No point continuing if the elf file doesn't exist
  if [ -f "$PWD/daedalus.elf" ]; then
    #Pack PBP
  psp-fixup-imports daedalus.elf
  mksfoex -d MEMSIZE=1 DaedalusX64 PARAM.SFO
  psp-prxgen daedalus.elf daedalus.prx
  cp ../Source/SysPSP/Resources/eboot_icons/* "$PWD"
  pack-pbp EBOOT.PBP PARAM.SFO icon0.png NULL NULL pic1.png NULL daedalus.prx NULL
  mv EBOOT.PBP ../DaedalusX64


fi
    }

## Main loop


if [ "$1" = "PSP_RELEASE" ] || [ "$1" = "PSP_DEBUG" ]; then
  cleanup
    cd "$PWD/daedbuild"
cmake -DCMAKE_TOOLCHAIN_FILE=../Tools/psptoolchain.cmake -D"$1=1" ../Source
buildPSP

elif [ "$1" = "LINUX_RELEASE" ] || [ "$1" = "MAC_RELEASE" ]; then
  cleanup
  cd "$PWD/daedbuild"
  cmake -D"$1=1" ../Source
make
finalPrep
cp daedalus ../DaedalusX64
else
usage
fi
