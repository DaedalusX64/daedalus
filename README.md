# DaedalusX64
 
DaedalusX64 is a Nintendo 64 emulator for PSP, 3DS, Vita, Linux, macOS and Windows
 
## Features:
 
- Fast emulation
- High compatibility
- Support for PSP TV Mode with 480p output!
- Active support and updates

## Things that don't work right now:
- Big Endian (So no builds for PowerPC at the moment sorry).
 
## Usage
 
Installing Daedalus:
Download the latest release for your platform from https://github.com/DaedalusX64/daedalus/releases or if you dare, use the latest builds in the GitHub Action section

Extra Steps:
PSP: Copy the entire DaedalusX64 Folder to your Memory Stick and put into PSP/Game

macOS / Linux:
 Prerequisites: libpng / zlib / minizip / glew / SDL2 (You'll need to do this via a package manager.
 Execute the emulator ./daedalus 'Path to Rom' 
Note - Native Apple Silicon build via GitHub is not available yet however it compiles fine and works (Just a bit slow as expected)
## Building
All Versions require these libraries: zlib, libpng, minizip
 ## Buiilding for PSP
    Fetch the latest PSP Toolchain from https://github.com/pspdev/pspdev and install required libraries
    
    Run    ./build_daedalus.sh PSP and CMake will do the required fun stuff :)

## Building for Windows 

NB: Windows build is broken at the moment 

1) Clone and open the repo in Visual studio 2019

2) Build All

## Building for Posix OS (Linux & macOS)
Posix OSes additionally require glew / SDL2

1) Clone this repo 

2) ./build_daedalus.sh (This will auto detect the OS you are on)
 
 ## CI
 DaedalusX64 now has CI, you can get the latest nightlies from the Actions Tab.
 Warning: these builds are sporatic at times.

## More Info
 
For information about compatibility, optimal settings and more about the emulator, visit the actively maintained GitHub wiki page: https://github.com/DaedalusX64/daedalus/wiki Feel free to submit reports for how well your favourite games run if they have not already been listed!
 
We're on Discord, come catch up with the latest news. Chaos ensured. https://discord.gg/FrVTpBV
 
## Credits
- StrmnNrmn - For bringing this project to us
- Kreationz, salvy, Corn, Chilly Willy, Azimer, Shinydude100 and missed folks for past contributions
- MasterFeizz for 3DS Support / ARM DynaRec
- Xerpi / Rinnegatamante / TheOfficialFloW for PS Vita and ARM Contributions
- z2442 & Wally: Compilation improvements and updating, optimizations
- TheMrIron2 & mrneo240 Optimizations, wiki maintenance
- re4thewin for continuous testing over the duration of DaedalusX64
- motolegacy for help with site and emulator itself
- fjtrujy for help with getting CI going.
- Our Valued Communities for helping making Daedalus what it is :)
