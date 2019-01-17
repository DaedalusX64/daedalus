#!/bin/bash




if [ -d $PWD/daedbuild ] ; then
echo "Removing Previous build attempt"
rm -r $PWD/daedbuild
make -C $PWD/Source/SysPSP/DveMgr clean
make -C $PWD/Source/SysPSP/ExceptionHandler/prx clean
make -C $PWD/Source/SysPSP/KernelButtonsPrx clean
make -C $PWD/Source/SysPSP/MediaEnginePRX

fi

#check for build directory
if  [ ! -d $PWD/daedbuild ] ; then
mkdir $PWD/daedbuild
fi


if [ -z "$1" -o "$1" != PSP_RELEASE ]; then
echo "Usage ./build_daedalus.sh BUILD_TYPE"
echo "Build Types:"
echo "PSP Release = PSP_RELEASE"
exit
fi


if [ "$1" = PSP_RELEASE ]; then


echo "Temporarily manually build some additional files via make"
make -C $PWD/Source/SysPSP/DveMgr
make -C $PWD/Source/SysPSP/ExceptionHandler/prx
make -C $PWD/Source/SysPSP/KernelButtonsPrx
make -C $PWD/Source/SysPSP/MediaEnginePRX
cd $PWD/daedbuild 
cmake -DCMAKE_TOOLCHAIN_FILE=../Tools/psptoolchain.cmake -D$1=1 ../Source
make
mkdir -p Daedalus
mv $PWD/daedalus.elf $PWD



mksfo ‘Daedalus’ PARAM.SFO
psp-strip daedalus.elf -o Daedalus_strip.elf
pack-pbp ../DaedalusX64/EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL Daedalus_strip.elf NULL

fi


