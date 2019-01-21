#!/bin/bash

if [ -d $PWD/daedbuild ] ; then
echo "Removing Previous build attempt"
rm -r $PWD/daedbuild
rm -r $PWD/DaedalusX64
make -C $PWD/Source/SysPSP/DveMgr clean
make -C $PWD/Source/SysPSP/ExceptionHandler/prx clean
make -C $PWD/Source/SysPSP/KernelButtonsPrx clean
make -C $PWD/Source/SysPSP/MediaEnginePRX clean

fi

#check for build directory
if  [ ! -d $PWD/daedbuild ] ; then
mkdir $PWD/daedbuild
fi



if [ "$1" = PSP_RELEASE -o "$1" = PSP_DEBUG ]; then
mkdir DaedalusX64
make -C $PWD/Source/SysPSP/DveMgr
make -C $PWD/Source/SysPSP/ExceptionHandler/prx
make -C $PWD/Source/SysPSP/KernelButtonsPrx
make -C $PWD/Source/SysPSP/MediaEnginePRX
cd $PWD/daedbuild
cmake -DCMAKE_TOOLCHAIN_FILE=../Tools/psptoolchain.cmake -D$1=1 ../Source
make

#No point continuing if the elf file doesn't exist
if [ -f $PWD/daedalus.elf ]; then 

psp-fixup-imports daedalus.elf
mksfoex -d MEMSIZE=1 DaedalusX64 PARAM.SFO
psp-prxgen daedalus.elf daedalus.prx
# USAGE: pack-pbp <output.pbp> <param.sfo> <icon0.png> <icon1.pmf> <pic0.png> <pic1.png> <snd0.at3> <data.psp> <data.psar>
cp ../Source/SysPSP/Resources/eboot_icons/* $PWD
pack-pbp EBOOT.PBP PARAM.SFO icon0.png NULL NULL pic1.png NULL daedalus.prx NULL
mv EBOOT.PBP ../DaedalusX64
cp -r ../Data/PSP/* ../DaedalusX64
mkdir ../DaedalusX64/SaveStates
mkdir ../DaedalusX64/SaveGames
mkdir ../DaedalusX64/Roms
fi
else
echo "Usage ./build_daedalus.sh BUILD_TYPE"
echo "Build Types:"
echo "PSP Release = PSP_RELEASE"
echo "PSP Debug = PSP_DEBUG"
exit
fi


