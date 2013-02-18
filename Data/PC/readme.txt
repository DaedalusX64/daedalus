Daedalus v0.07b Readme File
Copyright (C) 2001 StrmnNrmn

This document last edited 3 June 2001.

This is the binary distribution of Daedalus. If you are a
developer, you may find the source distribution more useful.

What is Daedalus?
*****************

Daedalus is a Nintendo64 emulator for Windows. Daedalus is named
after the craftsman at King Minos's court who designed the 
labyrinth for the Minotaur.

Getting the Latest Version
**************************

Daedalus is currently hosted on the Boob site at
http://daedalus.boob.co.uk/. The most recent version of
Daedalus will always be available there (and hopefully mirrored
on some of the other emulation sites).

Using Daedalus
***************

Loading Roms
------------

Daedalus should be very easy to use. To open a rom, use the
"File->Open" menu and select the name of the rom to load. The
rom is automatically executed. Daedalus remembers the last rom
you opened from the File menu and will list all the other roms
from that directory next time you start it up.

Configuring Audio and Graphics
------------------------------

Audio support is currently provided through Zilmar's audio
plugin spec. Thanks to Azimer, Daedalus now ships with a demo of his
forthcoming pluging for TrueReality. This demos plugin works with all
ABI 1 ucodes. You will need to set up this plugin for use using the
steps described below. 

Daedalus has built-in graphics handling, and it also supports plugins
that support Zilmar's plugin spec.

You can download alternative plugins from many sites. Emulation 64's
Plugin page (http://www.emulation64.com/plugin64.htm) has many plugins
available for download.

To configure audio, choose "Config->Audio Configuaration..." from
the menu. The dialog box will show any plugins that are present in 
the Plugins directory of Daedalus, so you will need to copy any
plugins there. From v0.03b, Daedalus should be capable of hot-swapping
audio plugins while the emulator is running. If you have any
difficulties please try restarting the emulator.

To configure graphics, choose "Config->Graphics Configuration..." from
the menu. In order to change graphics plugins while the emulator is
running, you will need to stop/start the CPU. In order to use
Daedalus's built-in graphics, disable the use of the plugin in the
Graphics Configuration dialog.
 [NOTES: At the moment there a few differences in the way that
  the built-in graphics and the plugins behave. Fullscreen mode
  is NOT YET supported for plugins, but probably will be fixed for
  the next release. Screen shots are not yet supported for the
  plugins. The status bar is disabled for the built-in graphics.]


Configuring Input
-----------------

Input is provided by DirectInput8 (you can configure controllers
using the "Gaming Options" control panel option). Within Daedalus, 
controller configuration is performed by selecting 
"Config->Input Configuration...". Daedalus now supports multiple
controllers. The default configuration is as follows:

Analogue Stick: Arrow Keys
Digital Pad   : T, F, G, V
C Buttons     : I, J, K, M
A             : A
B             : S
Z Trigger     : X
L Trigger     : [
R Trigger     : ]
Start         : Enter

Daedalus 0.04b onwards supports Input configuration files (.din files). These
can (theoretically) be swapped between users, so pre-made configurations
can be distributed easily. 

Saved States
------------

Daedalus currently supports Eeprom, Mempack and SRAM saves in the
same format as Nemu (.sav, .mpk and .sra respectively). 

To use other people's saved state with your roms, you must
rename them so they have the same filespec as the rom, and
place them in the same directory. For instance, for:

C:\Roms\wibble.rom

The eeprom would be

C:\Roms\wibble.sav

And the mempack would be

C:\Roms\wibble.mpk

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


Requirements
************

I have not tested Daedalus on anything lower than my development
machine, which is a P3-500, GeForce 2 MX with 128Mb RAM. 

Daedalus requires DirectX 8 (or 8a) to be installed, which you
can download from http://www.microsoft.com/directx/

For those that have asked:

Graphics:  Direct3D 7, Zilmar's plugin spec
Input   :  DirectInput 8
Audio   :  Zilmar's plugin spec

3DfX and ATi
************

There are still problems with Voodoo based cards. I have tried
as much as possible to remove the bugs that caused Daedalus to 
crash on 3dfx cards, but there are still many graphical glitches
remaining. Unfortunately as I do not have a Voodoo card of my own,
it is very difficult for me to test and fix bugs. Hopefully over
the coming weeks support for 3dfx cards should improve
substantially.

If you find that Daedalus causes problems with your graphics card,
you now have the option of trying one of the graphics plugins that
are available.


Support, Bugs, Comments etc
***************************

This version of Daedalus should be considered a BETA RELEASE and
as such it is anticipated that it will crash from time to time.

For support issues, bug reports, comments and so on, please visit
the Daedalus homepage:

http://daedalus.boob.co.uk/

Who / What is StrmnNrmn?
************************

StrmnNrmn is short for Storming Norman. "Storming Norman" is
slightly too long to use as a nick on IRC, so it has been
shortened to "StrmnNrmn".

Credits
*******

Thanks to CyRUS64 for prompting me to release the emulator,
for helping with some of the programming tasks and for hosting
the Daedalus site on Boob.

A very big thanks to the following people:

o Azimer, _Demo_, F|RES, Icepir8, Jabo, LaC, Lemmy, Niki Waibel,
  Zilmar, and all the other N64 emu developers who have put up with
  my awkward questions over the past few months!
o Bjz, CyRUS64 and Genueix for compiling the ini file with this release.
o A big thanks to JTS and others in #daedalus for helping to sort
  out the 3dfx bugs.
o Thanks to Gorxon and Genueix for maintaining the website (and
  The Gypsy for hosting!).
o Thanks to JTS for doing the FAQ.
o Thanks to Lkb for some very nice additions to the Debug Console
  and mirrored texture support for cards that don't support it
  in hardware. Lkb has also added much better localisation support,
  and been improving the Dynamic Recompiler.
o Thanks to Jun Su and Orkin for various other changes.

Check the credits.txt file for more details.


Copying
*******

Daedalus is released under the GNU General Public License,
and the sourcecode is available from the Boob site. 

Disclaimer
**********

The Daedalus distribution comes with absolutely no warranty of any kind.