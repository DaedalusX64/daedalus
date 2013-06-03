Daedalus v1.00 Readme File for PC/OSX/Linux
Copyright (C) 2001-2013 StrmnNrmn
Copyright (C) 2008-2013 DaedalusX64 Team

This document last edited 31 May 2013

This is the binary distribution of Daedalus. If you are a
developer, you may find the source distribution more useful.

What is Daedalus?
*****************

Daedalus is a Nintendo64 emulator for PSP/Windows/OSX/Linux. Daedalus is named
after the craftsman at King Minos's court who designed the 
labyrinth for the Minotaur.

Getting the Latest Version
**************************

Binaries are currently hosted on http://www.daedalusx64.com
Source can be found on https://github.com/hulkholden/daedalus
The most recent version of Daedalus will always be available there (and hopefully mirrored on some of the other emulation sites).

Using Daedalus
***************

Loading Roms
------------

Daedalus should be very easy to use. To open a rom, just drag it to the Daedalus executable.

Configuring Audio and Graphics
------------------------------

Daedalus has built-in graphics and audio handling, no futher configuration is required.

Configuring Input
-----------------

Input is provided by GLFW, gamepads are supported too (PS3 controller is recommended).
To use a gamepad, make sure is connected before starting Daedalus, also make sure the drivers for your gamepad are correctly installed.
Currently input configurations are not supported, also gamepad support is very limited.

Digital Pad   : Num 4, Num 6, Num 8, Num 2
C Buttons     : Delete, PageDown, Home, End
A             : X
B             : C
Z Trigger     : Z
Z Trigger     : Y (German keyboards)
L Trigger     : A
R Trigger     : S
Start         : Enter


Save Games
------------

IMPORTANT NOTE: For performance reasons Daedalus only saves out
modified save game every 60 vbl.

Daedalus supports Eeprom, Mempack, SRAM and FlashRam saves in the
same format as Nemu (.sav, .mpk and .sra respectively). 

To use other people's saves with your roms, you must
rename them so they have the same filespec as the rom, and
place them in the same directory. For instance, for:

C:\Roms\wibble.rom

The eeprom would be

C:\Roms\wibble.sav

And the mempack would be

C:\Roms\wibble.mpk


Saved States
------------

All the save states are created using this name format : SaveSlotXX.ss (XX can
be from 0 to 10) Save States are saved by default at
/SaveStates/<gamename>/.

To create a save state, press Ctrl 0-9 and for loading press 0-9.
Each number corresponds to each slot where the save state was created.
Each game has 10 slots to save.

Rom Support
-----------

Daedalus currently supports .v64, .z64, n64, .rom, .jap, .pal
and .usa formats (let me know if you know of any other common
filetypes). 
Daedalus largely ignores the filetype, but it will only
recognise files with these extensions as roms. You can
also archive roms into a .zip file and Daedalus will
automatically load the rom from there. Daedalus only
recongnises the FIRST rom in the zip file, so there is
no point in putting multiple roms into the same zipfile.


Minimun Requirements
************


Video card with support for atleast OpenGL 3.2
Please refer to your hardware vendor and make sure to always have the latest drivers installed


Support, Bugs, Comments ,Chat etc
***************************

This version of Daedalus should be considered a BETA RELEASE and as such, bugs are expected.

For support issues, bug reports, comments and so on, please visit
the Daedalus homepage:

www.DaedalusX64.com

irc.freenode.net #daedalusx64

Who / What is StrmnNrmn?
************************

StrmnNrmn is short for Storming Norman. "Storming Norman" is
slightly too long to use as a nick on IRC, so it has been
shortened to "StrmnNrmn".

Credits
*******

StrmnNrmns Credits:
********************

The Audio HLE code used in Daedalus was adapted from Azimer's great
plugin for PC-based emulators. Thanks Azimer! Drop me a line!

A special thanks to everyone who was involved with Daedalus on the PC in 
the past. Sorry I was so rubbish at keeping the project ticking along. You
know who you are.

Hello to everyone I used to know in the N64 emulation scene back in 2001 -
hope you are all doing well!


Many thanks to 71M for giving me the inspiration to get this ported over
and lots of pointers along the way.

A big hello to the Super Zamoyski Bros :)

Many thanks to all the people who suggested using the Circle button to 
toggle between the Dpad and C buttons. I received over two dozen emails and
comments suggesting this approach, so thanks for that :)

Many thanks to hlide and Raphael in the PS2Dev forums for advice on
various VFPU issues.

Many thanks to Exophase and laxer3A for their continued input.

Thanks to Lkb for his various improvements to the PC build. The current 
savestate support is derived from his work.

Daedalusx64 Credits:
*********************

A big thanks to all our testers, and believers for giving us a hand when we need it.
A huge thanks to StrmnNrmn for getting the ball rolling with this excellent emulator and for providing tips to our developers when he can.
And many thanks to the following developers who have helped in improving this emulator.

Howard0su
Salvy
Corn
Hlide
Grazz
Chilly Willy
Maxi_Jack
Wally
Kreationz

Copying
*******

Daedalus is released under the GNU General Public License, and the sourcecode is
available at https://github.com/hulkholden/daedalus.

Disclaimer
**********

The Daedalus distribution comes with absolutely no warranty of any kind.