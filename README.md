# DaedalusX64
 
DaedalusX64 is a Nintendo 64 emulator for PSP, 3DS, Vita, Linux, macOS and Windows
 
## Features:
 
- Fast emulation
- High compatibility
- Support for PSP TV Mode with 480p output!
- Active support and updates
- Lots of experimental optimizations
 
And more!
 
## Usage
 
To install Daedalus to your PSP, download the latest release from the Releases page: https://github.com/DaedalusX64/daedalus/releases
 
Next, plug your PSP into your computer and navigate to /PSP/GAME/. Create a folder called "daedalus" there, and place the EBOOT.PBP file inside. Place your ROM files in daedalus/Roms/ and they will automatically appear in Daedalus.
 
Note: If the release is a ZIP file with a folder containing an EBOOT.PBP file when extracted, simply drag and drop the extracted folder into /PSP/GAME/.
 
## Building
All Versions require these libraries: zlib, libpng, minizip
 ## Buiilding for PSP
PSP Version requires libintrafont 

## Building for Windows 

1) Clone and open the repo in Visual studio 2019

2) Build All

## Building for Linux and MAC

1) Clone this repo 

2) ./build_daedalus.sh
 
 ## CI
 DaedalusX64 now has CI, you can get the latest nightlies from the Actions Tab.
 Warning: these builds are sporatic at times.
## More Info
 
For information about compatibility, optimal settings and more about the emulator, visit the actively maintained GitHub wiki page: https://github.com/DaedalusX64/daedalus/wiki Feel free to submit reports for how well your favourite games run if they have not already been listed!
 
Join our Discord server to talk to other Daedalus users and the developers!
 
Invite link: https://discord.gg/FrVTpBV
 
## Credits
- StrmnNrmn - For making DaedalusX64 possible
- Kreationz, salvy, Corn, Chilly Willy, Azimer, missed folks for past contributions
- MasterFeizz for 3DS Support / ARM DynaRec
- Xerpi / Rinnegatamante / TheOfficialFloW for PS Vita and ARM Contributions
- z2442 & Wally: Compilation improvements and updating, optimizations
- TheMrIron2 & mrneo240 Optimizations, wiki maintenance
- re4thewin for continuous testing over the duration of DaedalusX64
- motolegacy for GitHub Website 
- fjtrujy for help with getting CI going.
- Our Valued Communities for helping making Daedalus what it is :)