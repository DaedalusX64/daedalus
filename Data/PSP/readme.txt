Daedalus PSP R13 Readme File
Copyright (C) 2006-2007 StrmnNrmn

This document last edited 04 November 2007

This is the binary distribution of Daedalus PSP. If you are a
developer, you may find the source distribution more useful.

What is Daedalus?
*****************

Daedalus is a Nintendo64 emulator for Windows and PSP. Daedalus
is named after the craftsman at King Minos's court who designed
the labyrinth for the Minotaur.

Getting the Latest Version
**************************

Daedalus is currently hosted on sourceforge at
http://sourceforge.net/projects/daedalus-n64. The most recent 
version of Daedalus will always be available there (and hopefully
mirrored on some of the other emulation/PSP sites).

About this Release
******************

This release of Daedalus for the PSP is currently in early stages
of development. It is intended to show the potential for a N64
emulator on the PSP, but the current release has many missing
features that make it far from usable:

* There is limited savegame support.
* Not all the N64 controls can be used simultaneously.
* Many roms won't boot.
* Many roms have serious graphical glitches.
* Many roms have random lockups and crashes.
* Many roms run at a low framerate.

Having said all that, I believe most of these problems can be
overcome in the future.

Using Daedalus
**************

Installation
------------

Copy the Daedalus folder to the \PSP\GAME\ folder on your PSP (for
the v1.5+ firmware release you should also copy across the Daedalus% 
folder). 

See the Homebrew FAQ on qj.net for more information on 
how to get homebrew running on your particular version of firmware:
http://forums.qj.net/showpost.php?p=14662&postcount=2

The PSP Homebrew FAQ is also a good reference:
http://www.psp-homebrew.eu/faq/index.php

Roms
----

Roms should be copied to the Roms subdirectory within the
Daedalus folder. Daedalus recognises most roms formats (.v64, .z64,
.rom etc) and will also run roms compressed within .zip files.

NB: Recent versions of Daedalus will also load any roms found
within the  \N64\ directory on your PSP. Note that this is in the
*root* directory of your PSP (e.g. for me it appears as P:\N64\ when 
I open the USB connection in Windows).

When Daedalus boots with a zipped rom, for performance reasons it
will attempt to decompress it to a temporary file on your memory 
stick before execution. If you are low on free space, it is 
recommended that you keep the original rom file on your PC, and 
manually extract the rom to your Roms directory.

Preview Pictures
----------------

Preview pictures for the rom selection screen can be added to the 
Resources\Preview directory in the Daedalus folder. They should be
in .png format, in a 4:3 aspect ratio.

To let Daedalus know which picture to use for each rom, you need to
add a line to the corresponding entry in the main daedalus.ini file,
with this format:

Preview=<filename.png>

Where <filename.png> is the name of the .png file. Don't include any
directory names in the path.

Save Games
----------

Currently Daedalus PSP has limited support for the following save
game types:

4Kb Eeprom
16Kb Eeprom
Mempack

Save games are created with the same name as the rom file, in the
Daedalus/SaveGames/ directory.

IMPORTANT NOTE: For performance reasons Daedalus only saves out
modified save game files when the Pause menu is accessed (by pressing
the 'Select' button while the emulator is running). I'll look at removing
this restriction ASAP, alongside adding SRAM support.

Main Menu
---------

When you run Daedalus, it will present you with a list of all the roms
it could find in your Roms subdirectory. Use the dpad or analogue stick
to navigate to the desired rom, and press X or Start to select it.
A new screen is displayed from which you can select to edit preferences
for the rom or start emulation. 

You can use the left and right shoulder buttons to cycle between other
options screens in Daedalus. 

Pause Menu
----------

When a rom is running, you can access the Pause Menu by pressing the 
'Select' button.

From the Pause Menu you can use the left and right shoulder buttons to
access various option screens. You can use the Pause Menu to take screenshots
and reset the emulator to the main menu. Screenshots are saved under
the Dumps/<gamename>/ScreenShots/ directory in the Daedalus folder on your
memory stick.

You can press the 'Select' button again to quickly return to the emulator.

Controls
--------

When (and indeed if :) the rom runs, the following controls are mapped
by default:

N64					PSP
Start				Start
Analogue Stick		Analogue Stick
Dpad				O (Circle) + Dpad
A					X (Cross)
B					[] (Square)
Z					^ (Triangle)
L Trigger			L Trigger
R Trigger			R Trigger
C buttons			Dpad (Circle unpressed)

As of R7 Daedalus now allows user-configurable controls to be specified.
The desired controls can be chosen from the Rom Settings screen.

In order to define your own controller configuration you need to add a 
new .ini file to the Daedalus/ControllerConfigs directory. There are a
few examples provided which should give an overview of what is possible.
I will look at providing a more thorough tutorial shortly.

Support, Bugs, Comments etc
***************************

If you feel the need to get in touch, I can be contacted at:

					strmnnrmn@gmail.com

I've been deluged with mail since the first release of this emulator,
so apologies if I don't get back to you immediately.

Credits
*******

The Audio HLE code used in Daedalus was adapted from Azimer's great
plugin for PC-based emulators. Thanks Azimer! Drop me a line!

A special thanks to everyone who was involved with Daedalus on the PC in 
the past. Sorry I was so rubbish at keeping the project ticking along. You
know who you are.

Hello to everyone I used to know in the N64 emulation scene back in 2001 -
hope you are all doing well!

A huge thanks to the pspdev guys for all their work.

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

Copying
*******

Daedalus is released under the GNU General Public License,
and the sourcecode is available from the sourceforge site. 

Disclaimer
**********

The Daedalus distribution comes with absolutely no warranty of any kind.

