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

// #include 

#include <string.h>

#include "Base/MathUtil.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/HLEAudioInternal.h"
#include "HLEAudio/HLEAudioState.h"

bool isMKABI = false;
bool isZeldaABI = false;

AudioHLEInstruction ABI2[0x20] = {
// std::array<AudioHLEInstruction, 0x20> ABI2 = {
    SPNOOP,    ADPCM2,      CLEARBUFF2, UNKNOWN,    ADDMIXER,  RESAMPLE2,
    UNKNOWN,   SEGMENT2,    SETBUFF2,   DUPLICATE2, DMEMMOVE2, LOADADPCM2,
    MIXER2,    INTERLEAVE2, HILOGAIN,   SETLOOP2,   SPNOOP,    DEINTERLEAVE2,
    ENVSETUP1, ENVMIXER2,   LOADBUFF2,  SAVEBUFF2,  ENVSETUP2, SPNOOP,
    HILOGAIN,  SPNOOP,      DUPLICATE2, UNKNOWN,    SPNOOP,    SPNOOP,
    SPNOOP,    SPNOOP};

/* NOTES:

  FILTER/SEGMENT - Still needs to be finished up... add FILTER?
  UNKNOWWN #27	 - Is this worth doing?  Looks like a pain in the ass just for
  WaveRace64
*/
