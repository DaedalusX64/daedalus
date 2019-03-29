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

void CLEARBUFF( AudioHLECommand command )
{
	u16 addr( command.Abi1ClearBuffer.Address );
	u16 count( command.Abi1ClearBuffer.Count );

gAudioHLEState.ClearBuffer( addr, count );
}

void CLEARBUFF2( AudioHLECommand command )
{
	u16 addr( command.Abi2ClearBuffer.Address );
	u16 count( command.Abi2ClearBuffer.Count );

	gAudioHLEState.ClearBuffer( addr, count );
}

void CLEARBUFF3( AudioHLECommand command )
{
	u16 addr = (u16)(command.cmd0 &  0xffff);
	u16 count = (u16)(command.cmd1 & 0xffff);
	memset(gAudioHLEState.Buffer + addr + 0x4f0, 0, count);
}

void DMEMMOVE( AudioHLECommand command )
{
	u16 src( command.Abi1DmemMove.Src );
	u16 dst( command.Abi1DmemMove.Dst );
	u16 count( command.Abi1DmemMove.Count );

	gAudioHLEState.DmemMove( dst, src, count );
}

void DMEMMOVE2( AudioHLECommand command )
{
	// Needs accuracy verification...
	u16 src( command.Abi2DmemMove.Src );
	u16 dst( command.Abi2DmemMove.Dst );
	u16 count( command.Abi2DmemMove.Count );

	gAudioHLEState.DmemMove( dst, src, count );
}

// Needs accuracy verification...
void DMEMMOVE3( AudioHLECommand command )
{
	u16 src( command.Abi3DmemMove.Src );
	u16 dst( command.Abi3DmemMove.Dst );
	u16 count( command.Abi3DmemMove.Count );

	gAudioHLEState.DmemMove( dst + 0x4f0, src + 0x4f0, count );
}

void DUPLICATE2( AudioHLECommand command )
{
	u32 Count {(command.cmd0 >> 16) & 0xff};
	u32 In  {command.cmd0&0xffff};
	u32 Out {command.cmd1>>16};

	u16 buff[64] {};

	memcpy(buff, gAudioHLEState.Buffer +In, 128);

	while(Count)
	{
		memcpy(gAudioHLEState.Buffer + Out, buff, 128);
		Out+=128;
		Count--;
	}
}

void LOADBUFF( AudioHLECommand command )
{
	u32 addr(command.Abi1LoadBuffer.Address );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.LoadBuffer( addr );
}

void LOADBUFF2( AudioHLECommand command )
{
	// Needs accuracy verification...
	u32 src( command.Abi2LoadBuffer.SrcAddr );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];
	u16 dst( command.Abi2LoadBuffer.DstAddr );
	u16 count( command.Abi2LoadBuffer.Count );

	gAudioHLEState.LoadBuffer( dst, src, count );
}

void LOADBUFF3( AudioHLECommand command )
{
	u32 v0 {};
	u32 cnt {(((command.cmd0 >> 0xC)+3)&0xFFC)};
	v0 = (command.cmd1 & 0xfffffc);
	u32 src {(command.cmd0&0xffc)+0x4f0};
	memcpy (gAudioHLEState.Buffer+src, rdram+v0, cnt);

}

// memcpy causes static... endianess issue :(
void SAVEBUFF( AudioHLECommand command )
{
	u32 addr(command.Abi1SaveBuffer.Address );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.SaveBuffer( addr );
}

void SAVEBUFF2( AudioHLECommand command )
{
	// Needs accuracy verification...
	u32 dst( command.Abi2SaveBuffer.DstAddr );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];
	u16	src( command.Abi2SaveBuffer.SrcAddr );
	u16 count( command.Abi2SaveBuffer.Count );

	gAudioHLEState.SaveBuffer( dst, src, count );
}

void SAVEBUFF3( AudioHLECommand command )
{
	u32 v0 {};
	u32 cnt {(((command.cmd0 >> 0xC)+3)&0xFFC)};
	v0 = (command.cmd1 & 0xfffffc);
	u32 src {(command.cmd0&0xffc)+0x4f0};

	memcpy (rdram+v0, gAudioHLEState.Buffer+src, cnt);

}

/*
void SEGMENT( AudioHLECommand command )
{
	u8	segment( gAudioHLEState.Abi1SetSegment.Segment & 0xf );
	u32	address( gAudioHLEState.Abi1SetSegment.Address );

	gAudioHLEState.Abi1SetSegment( segment, address );
}
*/
void SEGMENT2( AudioHLECommand command ) {
	if (isZeldaABI) {
		FILTER2( command );
		return;
	}
	if ((command.cmd0 & 0xffffff) == 0) {
		isMKABI = true;
		//gAudioHLEState.Segments[(command.cmd1>>24)&0xf] = (command.cmd1 & 0xffffff);
	} else {
		isMKABI = false;
		isZeldaABI = true;
		FILTER2( command );
	}
}

void SETBUFF( AudioHLECommand command )
{
	u8		flags( command.Abi1SetBuffer.Flags );
	u16		in( command.Abi1SetBuffer.In );
	u16		out( command.Abi1SetBuffer.Out );
	u16		count( command.Abi1SetBuffer.Count );

	if (flags & 0x8)
	{
		// A_AUX - Auxillary Sound Buffer Settings
		gAudioHLEState.AuxA = in;
		gAudioHLEState.AuxC = out;
		gAudioHLEState.AuxE = count;
	}
	else
	{
		// A_MAIN - Main Sound Buffer Settings
		gAudioHLEState.InBuffer  = in;
		gAudioHLEState.OutBuffer = out;
		gAudioHLEState.Count	  = count;
	}
}

void SETBUFF2( AudioHLECommand command )
{
	u16		in( command.Abi2SetBuffer.In );
	u16		out( command.Abi2SetBuffer.Out );
	u16		count( command.Abi2SetBuffer.Count );

#ifdef DAEDALUS_ENABLE_ASSERTS
	u8		flags( command.Abi2SetBuffer.Flags );
	DAEDALUS_ASSERT( flags == 0, "SETBUFF flags set: %02x", flags );
#endif

	gAudioHLEState.SetBuffer( 0, in, out, count );
}

void SETLOOP( AudioHLECommand command )
{
	u32	loopval( command.Abi1SetLoop.LoopVal );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.SetLoop( loopval );
}

void SETLOOP2( AudioHLECommand command )
{
	u32	loopval( command.Abi2SetLoop.LoopVal );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.SetLoop( loopval );
}

void SETLOOP3( AudioHLECommand command )
{
	u32	loopval( command.Abi3SetLoop.LoopVal );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.SetLoop( loopval );
}
