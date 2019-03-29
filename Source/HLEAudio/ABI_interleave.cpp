/*
Copyright (C) 2003 Azimer
Copyright (C) 2001,2006-2007 StrmnNrmn

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

#include "Core/Memory.h"
#include "Math/MathUtil.h"

#include "Debug/DBGConsole.h"
extern bool isZeldaABI;
extern bool isMKABI;

// Readded 26/03/19 - Looks correct
// XXX Saturate should probably be replaced with Azi's function for consistency
void ADDMIXER( AudioHLECommand command )
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
void HILOGAIN( AudioHLECommand command )
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

// Works... - 3-11-01
void INTERLEAVE( AudioHLECommand command )
{
	u16 inL( command.Abi1Interleave.LAddr );
	u16 inR( command.Abi1Interleave.RAddr );

	gAudioHLEState.Interleave( inL, inR );
}



void INTERLEAVE2( AudioHLECommand command )  // Needs accuracy verification...
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

void INTERLEAVE3( AudioHLECommand command )
{
	// Needs accuracy verification...
	//inR = command.cmd1 & 0xFFFF;
	//inL = (command.cmd1 >> 16) & 0xFFFF;

	gAudioHLEState.Interleave( 0x4f0, 0x9d0, 0xb40, 0x170 );
}


// Fixed a sign issue... 03-14-01
void MIXER( AudioHLECommand command )
{
	u16 dmemin( command.Abi1Mixer.DmemIn );
	u16 dmemout( command.Abi1Mixer.DmemOut );
	s32 gain( command.Abi1Mixer.Gain );

	gAudioHLEState.Mixer( dmemout, dmemin, gain );
}

void MIXER2( AudioHLECommand command )
{
	// Needs accuracy verification...
	u16 dmemin( command.Abi2Mixer.DmemIn );
	u16 dmemout( command.Abi2Mixer.DmemOut );
	s32 gain( command.Abi2Mixer.Gain );
	u16	count( command.Abi2Mixer.Count * 16 );

	//printf( "Mixer: i:%04x o:%04x g:%08x (%d) c:%04x - %08x%08x\n", dmemin, dmemout, gain, s16(gain), count, command.cmd0, command.cmd1 );

	gAudioHLEState.Mixer( dmemout, dmemin, gain, count );		// NB - did mult gain by 2 above, then shifted by 16 inside mixer.
}

void MIXER3( AudioHLECommand command )
{
	// Needs accuracy verification...
	u16 dmemin  = (u16)(command.cmd1 >> 0x10)  + 0x4f0;
	u16 dmemout = (u16)(command.cmd1 & 0xFFFF) + 0x4f0;
	//u8  flags   = (u8)((command.cmd0 >> 16) & 0xff);
	s32 gain    = (s16)(command.cmd0 & 0xFFFF);

	gAudioHLEState.Mixer( dmemout, dmemin, gain, 0x170 );		// NB - did mult gain by 2 above, then shifted by 16 inside mixer.
}
