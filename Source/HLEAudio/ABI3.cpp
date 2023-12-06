/*
Copyright (C) 2003 Azimer
Copyright (C) 2001,2006 StrmnNrmn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//
//	N.B. This source code is derived from Azimer's Audio plugin (v0.55?)
//	and modified by StrmnNrmn to work with Daedalus PSP. Thanks Azimer!
//	Drop me a line if you get chance :)
//
#include "Base/Types.h"


#include <string.h>

#include "HLEAudio/HLEAudioInternal.h"
#include "HLEAudio/HLEAudioState.h"

// Disables the command because it's not used?
static void DISABLE(AudioHLECommand) {}
static void WHATISTHIS(AudioHLECommand) {}

static void MP3ADDY(AudioHLECommand) {
  //	setaddr = (command.cmd1 & 0xffffff);
}

AudioHLEInstruction ABI3[0x20] = {
// std::array<AudioHLEInstruction, 0x20> ABI3 = {
    DISABLE,   ADPCM3,      CLEARBUFF3, ENVMIXER3, LOADBUFF3, RESAMPLE3,
    SAVEBUFF3, MP3,         MP3ADDY,    SETVOL3,   DMEMMOVE3, LOADADPCM3,
    MIXER3,    INTERLEAVE3, WHATISTHIS, SETLOOP3,  SPNOOP,    SPNOOP,
    SPNOOP,    SPNOOP,      SPNOOP,     SPNOOP,    SPNOOP,    SPNOOP,
    SPNOOP,    SPNOOP,      SPNOOP,     SPNOOP,    SPNOOP,    SPNOOP,
    SPNOOP,    SPNOOP,
};
