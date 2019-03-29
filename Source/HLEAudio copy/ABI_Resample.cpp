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
#include "AudioTypes.h"

extern bool isMKABI;
extern bool isZeldaABI;

inline s32		FixedPointMul16( s32 a, s32 b )
{
	return s32( ( a * b ) >> 16 );
}


void RESAMPLE( AudioHLECommand command )
{
	u8 flags(command.Abi1Resample.Flags);
	u32 pitch(command.Abi1Resample.Pitch);
	u32 address(command.Abi1Resample.Address);// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( (flags & 0x2) == 0, "Resample: unhandled flags %02x", flags );		// Was breakpoint - StrmnNrmn
	#endif
	pitch *= 2;

	s16 *	in ( (s16 *)(gAudioHLEState.Buffer) );
	u32 *	out( (u32 *)(gAudioHLEState.Buffer) );	//Save some bandwith and fuse two sample in one write
	u32		srcPtr((gAudioHLEState.InBuffer / 2) - 1);
	u32		dstPtr(gAudioHLEState.OutBuffer / 4);

	u32 accumulator {}, tmp {};

	if (flags & 0x1)
	{
		in[srcPtr^1] = 0;
		accumulator = 0;
	}
	else
	{
		in[(srcPtr)^1] = ((u16 *)rdram)[((address >> 1))^1];
		accumulator = *(u16 *)(rdram + address + 10);
	}

	for(u32 i = (((gAudioHLEState.Count + 0xF) & 0xFFF0) >> 2); i != 0 ; i-- )
	{
		tmp =  (in[srcPtr^1] + FixedPointMul16( in[(srcPtr+1)^1] - in[srcPtr^1], accumulator )) << 16;
		accumulator += pitch;
		srcPtr += accumulator >> 16;
		accumulator &= 0xFFFF;

		tmp |= (in[srcPtr^1] + FixedPointMul16( in[(srcPtr+1)^1] - in[srcPtr^1], accumulator )) & 0xFFFF;
		accumulator += pitch;
		srcPtr += accumulator >> 16;
		accumulator &= 0xFFFF;

		out[dstPtr++] = tmp;
	}

	((u16 *)rdram)[((address >> 1))^1] = in[srcPtr^1];
	*(u16 *)(rdram + address + 10) = (u16)accumulator;

	gAudioHLEState.Resample( flags, pitch, address );
}

void RESAMPLE2( AudioHLECommand command )
{
	u8 flags(command.Abi2Resample.Flags);
	u32 pitch(command.Abi2Resample.Pitch);
	u32 address(command.Abi2Resample.Address);// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.Resample( flags, pitch, address );
}

void RESAMPLE3( AudioHLECommand command )
{
	u8 Flags {(u8)((command.cmd1>>0x1e))};
	u32 Pitch {((command.cmd1>>0xe)&0xffff) << 1};
	u32 addy {(command.cmd0 & 0xffffff)};
	u32 Accum {};
	s16 *dst {};
	s16 *src {};
	dst = (s16 *)(gAudioHLEState.Buffer);
	src = (s16 *)(gAudioHLEState.Buffer);
	u32 srcPtr{ ((((command.cmd1>>2)&0xfff)+0x4f0)/2)};
	u32 dstPtr {};//=(gAudioHLEState.OutBuffer/2);

	srcPtr -= 1;

	if (command.cmd1 & 0x3) {
		dstPtr = 0x660/2;
	} else {
		dstPtr = 0x4f0/2;
	}

	if ((Flags & 0x1) == 0) {
		src[srcPtr^1] = ((u16 *)rdram)[((addy/2))^1];
		Accum = *(u16 *)(rdram+addy+10);
	} else {
		src[(srcPtr)^1] = 0;
		Accum = 0;
	}

	for(u32 i {} ; i < 0x170/2;i++)
	{
		dst[dstPtr^1] = src[srcPtr^1] + FixedPointMul16( src[(srcPtr+1)^1] - src[srcPtr^1], Accum );
		++dstPtr;
		Accum += Pitch;
		srcPtr += (Accum>>16);
		Accum &= 0xFFFF;
	}

	((u16 *)rdram)[((addy/2))^1] = src[srcPtr^1];
	*(u16 *)(rdram+addy+10) = u16( Accum );
}
