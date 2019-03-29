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

#include "stdafx.h"

#include <string.h>

#include "audiohle.h"
#include "AudioHLEProcessor.h"

#include "Math/MathUtil.h"

#include "Debug/DBGConsole.h"

static void SPNOOP( AudioHLECommand command )
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DBGConsole_Msg( 0, "AudioHLE: Unknown/Unimplemented Audio Command %i in ABI 2", command.cmd );
	#endif
}

bool isMKABI {false};
bool isZeldaABI {false};

static u32 gEnv_t3 {}, gEnv_s5 {}, gEnv_s6 {};
static u16 env[8] {};

static void MIXER2( AudioHLECommand command )
{
	// Needs accuracy verification...
	u16 dmemin( command.Abi2Mixer.DmemIn );
	u16 dmemout( command.Abi2Mixer.DmemOut );
	s32 gain( command.Abi2Mixer.Gain );
	u16	count( command.Abi2Mixer.Count * 16 );

	//printf( "Mixer: i:%04x o:%04x g:%08x (%d) c:%04x - %08x%08x\n", dmemin, dmemout, gain, s16(gain), count, command.cmd0, command.cmd1 );

	gAudioHLEState.Mixer( dmemout, dmemin, gain, count );		// NB - did mult gain by 2 above, then shifted by 16 inside mixer.
}

static void RESAMPLE2( AudioHLECommand command )
{
	u8 flags(command.Abi2Resample.Flags);
	u32 pitch(command.Abi2Resample.Pitch);
	u32 address(command.Abi2Resample.Address);// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.Resample( flags, pitch, address );
}



// OK 26/03/19 - Wally

static void DEINTERLEAVE2( AudioHLECommand command )
{
	u16 count( command.Abi2Deinterleave.Count );
	u16 out( command.Abi2Deinterleave.Out );
	u16 in( command.Abi2Deinterleave.In );

	gAudioHLEState.Deinterleave( out, in, count );
}

static void INTERLEAVE2( AudioHLECommand command )  // Needs accuracy verification...
{
	u16	inR( command.Abi2Interleave.RAddr );
	u16	inL( command.Abi2Interleave.LAddr);
	u16 out( command.Abi2Interleave.OutAddr );
	u16 count( command.Abi2Interleave.Count );

	if (count != 0)
	{
		gAudioHLEState.Interleave( out, inL, inR, count );
	}
	else
	{
		gAudioHLEState.Interleave( inL, inR );
	}
}

// Readded 26/03/19 - Looks correct
// XXX Saturate should probably be replaced with Azi's function for consistency
static void ADDMIXER( AudioHLECommand command )
{

	u32 Count     = (command.cmd0 >> 12) & 0x00ff0;
	u32 InBuffer  = (command.cmd1 >> 16);
	u32 OutBuffer = command.cmd1 & 0xffff;

	s16 *inp  = (s16 *)(gAudioHLEState.Buffer + InBuffer);
	s16 *outp = (s16 *)(gAudioHLEState.Buffer + OutBuffer);
	s32 temp;
	for (u32 cntr = 0; cntr < Count; cntr+=2)
	{

	s32 temp = Saturate<s16>( *outp + *inp );
		 *outp = temp;
		outp++;	inp++;
	}
}

// XXX Looks Correct 26/03/19
// Again with saturate
static void HILOGAIN( AudioHLECommand command )
{
	u16 count {command.cmd0 & 0xffff};
	u16 out {(command.cmd1 >> 16) & 0xffff};
	s16 hi  {(s16)((command.cmd0 >> 4) & 0xf000)};
	u16 lo  {(command.cmd0 >> 20) & 0xf};

	s16 *src = (s16 *)(gAudioHLEState.Buffer+out);

	while( count )
	{
		s32 val = (s32)*src;
		s32 tmp = ((val * (s32)hi) >> 16) + (u32)(val * lo);
		*src++ = Saturate<s16>( tmp );
		count -= 2;
	}
}


static void UNKNOWN( AudioHLECommand command )
{
}

AudioHLEInstruction ABI2[0x20] =
{
    SPNOOP , ADPCM2, CLEARBUFF2, UNKNOWN, ADDMIXER, RESAMPLE2, UNKNOWN, SEGMENT2,
    SETBUFF2 , DUPLICATE2, DMEMMOVE2, LOADADPCM2, MIXER2, INTERLEAVE2, HILOGAIN, SETLOOP2,
    SPNOOP, DEINTERLEAVE2 , ENVSETUP1, ENVMIXER2, LOADBUFF2, SAVEBUFF2, ENVSETUP2, SPNOOP,
    HILOGAIN , SPNOOP, DUPLICATE2 , UNKNOWN    , SPNOOP  , SPNOOP    , SPNOOP  , SPNOOP
};

/* NOTES:

  FILTER/SEGMENT - Still needs to be finished up... add FILTER?
  UNKNOWWN #27	 - Is this worth doing?  Looks like a pain in the ass just for WaveRace64
*/
