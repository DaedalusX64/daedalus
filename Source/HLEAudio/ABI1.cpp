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


#include "Base/MathUtil.h"
#include "HLEAudio/HLEAudioInternal.h"
#include "HLEAudio/HLEAudioState.h"

AudioHLEInstruction ABI1[0x20] = {
// std::array<AudioHLEInstruction, 0x20> ABI1 = {
    SPNOOP,  ADPCM,   CLEARBUFF, ENVMIXER, LOADBUFF,  RESAMPLE, SAVEBUFF,
    UNKNOWN, SETBUFF, SETVOL,    DMEMMOVE, LOADADPCM, MIXER,    INTERLEAVE,
    UNKNOWN, SETLOOP, SPNOOP,    SPNOOP,   SPNOOP,    SPNOOP,   SPNOOP,
    SPNOOP,  SPNOOP,  SPNOOP,    SPNOOP,   SPNOOP,    SPNOOP,   SPNOOP,
    SPNOOP,  SPNOOP,  SPNOOP,    SPNOOP};
