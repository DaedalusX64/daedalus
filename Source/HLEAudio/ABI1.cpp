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

#include "audiohle.h"
#include "AudioHLEProcessor.h"

#include "Math/MathUtil.h"

/******** DMEM Memory Map for ABI 1 ***************
Address/Range		Description
-------------		-------------------------------
0x000..0x2BF		UCodeData
	0x000-0x00F		Constants  - 0000 0001 0002 FFFF 0020 0800 7FFF 4000
	0x010-0x02F		Function Jump Table (16 Functions * 2 bytes each = 32) 0x20
	0x030-0x03F		Constants  - F000 0F00 00F0 000F 0001 0010 0100 1000
	0x040-0x03F		Used by the Envelope Mixer (But what for?)
	0x070-0x07F		Used by the Envelope Mixer (But what for?)
0x2C0..0x31F		<Unknown>
0x320..0x35F		Segments
0x360				Audio In Buffer (Location)
0x362				Audio Out Buffer (Location)
0x364				Audio Buffer Size (Location)
0x366				Initial Volume for Left Channel
0x368				Initial Volume for Right Channel
0x36A				Auxillary Buffer #1 (Location)
0x36C				Auxillary Buffer #2 (Location)
0x36E				Auxillary Buffer #3 (Location)
0x370				Loop Value (shared location)
0x370				Target Volume (Left)
0x372				Ramp?? (Left)
0x374				Rate?? (Left)
0x376				Target Volume (Right)
0x378				Ramp?? (Right)
0x37A				Rate?? (Right)
0x37C				Dry??
0x37E				Wet??
0x380..0x4BF		Alist data
0x4C0..0x4FF		ADPCM CodeBook
0x500..0x5BF		<Unknown>
0x5C0..0xF7F		Buffers...
0xF80..0xFFF		<Unknown>
***************************************************/

static void SPNOOP( AudioHLECommand command )
{
	//DBGConsole_Msg( 0, "AudioHLE: Unknown/Unimplemented Audio Command %i in ABI 1", command.cmd );
}

void CLEARBUFF( AudioHLECommand command )
{
	u16 addr( command.Abi1ClearBuffer.Address );
	u16 count( command.Abi1ClearBuffer.Count );

	gAudioHLEState.ClearBuffer( addr, count );
}

//FILE *dfile = fopen ("d:\\envmix.txt", "wt");

static void ENVMIXER( AudioHLECommand command )
{
	//static int envmixcnt = 0;
	u8	flags( command.Abi1EnvMixer.Flags );
	u32 address( command.Abi1EnvMixer.Address );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.EnvMixer( flags, address );
}

static void RESAMPLE( AudioHLECommand command )
{
	u8 flags(command.Abi1Resample.Flags);
	u32 pitch(command.Abi1Resample.Pitch);
	u32 address(command.Abi1Resample.Address);// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.Resample( flags, pitch, address );
}

static void SETVOL( AudioHLECommand command )
{
// Might be better to unpack these depending on the flags...
	u8 flags {(u8)((command.cmd0 >> 16) & 0xff)};
	s16 vol {(s16)(command.cmd0 & 0xffff)};
//	u16 voltgt =(u16)((command.cmd1 >> 16)&0xffff);
	u16 volrate = (u16)((command.cmd1 & 0xffff));

	if (flags & A_AUX)
	{
		gAudioHLEState.EnvDry = vol;				// m_MainVol
		gAudioHLEState.EnvWet = (s16)volrate;		// m_AuxVol
	}
	else if(flags & A_VOL)
	{
		// Set the Source(start) Volumes
		if(flags & A_LEFT)
		{
			gAudioHLEState.VolLeft = vol;
		}
		else
		{
			// A_RIGHT
			gAudioHLEState.VolRight = vol;
		}
	}
	else
	{
		// Set the Ramping values Target, Ramp
		if(flags & A_LEFT)
		{
			gAudioHLEState.VolTrgLeft  = (s16)(command.cmd0 & 0xffff);		// m_LeftVol
			gAudioHLEState.VolRampLeft = command.cmd1;
		}
		else
		{
			// A_RIGHT
			gAudioHLEState.VolTrgRight  = (s16)(command.cmd0 & 0xffff);		// m_RightVol
			gAudioHLEState.VolRampRight = command.cmd1;
		}
	}
}

static void UNKNOWN( AudioHLECommand command )
{
}

static void SETLOOP( AudioHLECommand command )
{
	u32	loopval( command.Abi1SetLoop.LoopVal );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.SetLoop( loopval );
}

static void ADPCM( AudioHLECommand command ) // Work in progress! :)
{
	u8		flags( command.Abi1ADPCM.Flags );
	//u16	gain( command.Abi1ADPCM.Gain );		// Not used?
	u32		address( command.Abi1ADPCM.Address );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.ADPCMDecode( flags, address );
}

// memcpy causes static... endianess issue :(
static void LOADBUFF( AudioHLECommand command )
{
	u32 addr(command.Abi1LoadBuffer.Address );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.LoadBuffer( addr );
}

// memcpy causes static... endianess issue :(
static void SAVEBUFF( AudioHLECommand command )
{
	u32 addr(command.Abi1SaveBuffer.Address );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.SaveBuffer( addr );
}

// Should work
/*
static void SEGMENT( AudioHLECommand command )
{
	u8	segment( command.Abi1SetSegment.Segment & 0xf );
	u32	address( command.Abi1SetSegment.Address );

	gAudioHLEState.SetSegment( segment, address );
}
*/
// Should work ;-)
static void SETBUFF( AudioHLECommand command )
{
	u8		flags( command.Abi1SetBuffer.Flags );
	u16		in( command.Abi1SetBuffer.In );
	u16		out( command.Abi1SetBuffer.Out );
	u16		count( command.Abi1SetBuffer.Count );

	gAudioHLEState.SetBuffer( flags, in, out, count );
}

// Doesn't sound just right?... will fix when HLE is ready - 03-11-01
static void DMEMMOVE( AudioHLECommand command )
{
	u16 src( command.Abi1DmemMove.Src );
	u16 dst( command.Abi1DmemMove.Dst );
	u16 count( command.Abi1DmemMove.Count );

	gAudioHLEState.DmemMove( dst, src, count );
}

// Loads an ADPCM table - Works 100% Now 03-13-01
static void LOADADPCM( AudioHLECommand command )
{
	u32		address(command.Abi1LoadADPCM.Address );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];
	u16		count( command.Abi1LoadADPCM.Count );

	gAudioHLEState.LoadADPCM( address, count );
}

// Works... - 3-11-01
static void INTERLEAVE( AudioHLECommand command )
{
	u16 inL( command.Abi1Interleave.LAddr );
	u16 inR( command.Abi1Interleave.RAddr );

	gAudioHLEState.Interleave( inL, inR );
}

// Fixed a sign issue... 03-14-01
static void MIXER( AudioHLECommand command )
{
	u16 dmemin( command.Abi1Mixer.DmemIn );
	u16 dmemout( command.Abi1Mixer.DmemOut );
	s32 gain( command.Abi1Mixer.Gain );

	gAudioHLEState.Mixer( dmemout, dmemin, gain );
}


// TOP Performance Hogs:
//Command: ADPCM    - Calls:  48 - Total Time: 331226 - Avg Time:  6900.54 - Percent: 31.53%
//Command: ENVMIXER - Calls:  48 - Total Time: 408563 - Avg Time:  8511.73 - Percent: 38.90%
//Command: LOADBUFF - Calls:  56 - Total Time:  21551 - Avg Time:   384.84 - Percent:  2.05%
//Command: RESAMPLE - Calls:  48 - Total Time: 225922 - Avg Time:  4706.71 - Percent: 21.51%

//Command: ADPCM    - Calls:  48 - Total Time: 391600 - Avg Time:  8158.33 - Percent: 32.52%
//Command: ENVMIXER - Calls:  48 - Total Time: 444091 - Avg Time:  9251.90 - Percent: 36.88%
//Command: LOADBUFF - Calls:  58 - Total Time:  29945 - Avg Time:   516.29 - Percent:  2.49%
//Command: RESAMPLE - Calls:  48 - Total Time: 276354 - Avg Time:  5757.38 - Percent: 22.95%

// TOP Performace Hogs: MIXER, RESAMPLE, ENVMIXER
AudioHLEInstruction ABI1[0x20] =
{
    SPNOOP , ADPCM , CLEARBUFF,	ENVMIXER  , LOADBUFF, RESAMPLE  , SAVEBUFF, UNKNOWN,
	SETBUFF, SETVOL, DMEMMOVE , LOADADPCM , MIXER   , INTERLEAVE, UNKNOWN , SETLOOP,
    SPNOOP , SPNOOP, SPNOOP   , SPNOOP    , SPNOOP  , SPNOOP    , SPNOOP  , SPNOOP,
    SPNOOP , SPNOOP, SPNOOP   , SPNOOP    , SPNOOP  , SPNOOP    , SPNOOP  , SPNOOP
};
