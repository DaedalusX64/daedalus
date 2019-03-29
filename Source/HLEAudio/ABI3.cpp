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

#include "Debug/DBGConsole.h"

#include "Math/MathUtil.h"
#include "Utility/FastMemcpy.h"

inline s32		FixedPointMul16( s32 a, s32 b )
{
	return s32( ( a * b ) >> 16 );
}

static void SPNOOP( AudioHLECommand command )
{
	DBGConsole_Msg( 0, "AudioHLE: Unknown/Unimplemented Audio Command %i in ABI 3", command.cmd );
}


static void MIXER3( AudioHLECommand command )
{
	// Needs accuracy verification...
	u16 dmemin  = (u16)(command.cmd1 >> 0x10)  + 0x4f0;
	u16 dmemout = (u16)(command.cmd1 & 0xFFFF) + 0x4f0;
	//u8  flags   = (u8)((command.cmd0 >> 16) & 0xff);
	s32 gain    = (s16)(command.cmd0 & 0xFFFF);

	gAudioHLEState.Mixer( dmemout, dmemin, gain, 0x170 );		// NB - did mult gain by 2 above, then shifted by 16 inside mixer.
}









static void RESAMPLE3( AudioHLECommand command )
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

static void INTERLEAVE3( AudioHLECommand command )
{
	// Needs accuracy verification...
	//inR = command.cmd1 & 0xFFFF;
	//inL = (command.cmd1 >> 16) & 0xFFFF;

	gAudioHLEState.Interleave( 0x4f0, 0x9d0, 0xb40, 0x170 );
}

/*
typedef struct {
	u8 sync;

	u8 error_protection	: 1;	//  0=yes, 1=no
	u8 lay				: 2;	// 4-lay = layerI, II or III
	u8 version			: 1;	// 3=mpeg 1.0, 2=mpeg 2.5 0=mpeg 2.0
	u8 sync2			: 4;

	u8 extension		: 1;    // Unknown
	u8 padding			: 1;    // padding
	u8 sampling_freq	: 2;	// see table below
	u8 bitrate_index	: 4;	//     see table below

	u8 emphasis			: 2;	//see table below
	u8 original			: 1;	// 0=no 1=yes
	u8 copyright		: 1;	// 0=no 1=yes
	u8 mode_ext			: 2;    // used with "joint stereo" mode
	u8 mode				: 2;    // Channel Mode
} mp3struct;

mp3struct mp3;
FILE *mp3dat;
*/

static void WHATISTHIS( AudioHLECommand command )
{
}

//static FILE *fp = fopen ("d:\\mp3info.txt", "wt");
//u32 setaddr;
static void MP3ADDY( AudioHLECommand command )
{
	//setaddr = (command.cmd1 & 0xffffff);
}


void MP3( AudioHLECommand command );


/*

FFT = Fast Fourier Transform
DCT = Discrete Cosine Transform
MPEG-1 Layer 3 retains Layer 2�s 1152-sample window, as well as the FFT polyphase filter for
backward compatibility, but adds a modified DCT filter. DCT�s advantages over DFTs (discrete
Fourier transforms) include half as many multiply-accumulate operations and half the
generated coefficients because the sinusoidal portion of the calculation is absent, and DCT
generally involves simpler math. The finite lengths of a conventional DCTs� bandpass impulse
responses, however, may result in block-boundary effects. MDCTs overlap the analysis blocks
and lowpass-filter the decoded audio to remove aliases, eliminating these effects. MDCTs also
have a higher transform coding gain than the standard DCT, and their basic functions
correspond to better bandpass response.

MPEG-1 Layer 3�s DCT sub-bands are unequally sized, and correspond to the human auditory
system�s critical bands. In Layer 3 decoders must support both constant- and variable-bit-rate
bit streams. (However, many Layer 1 and 2 decoders also handle variable bit rates). Finally,
Layer 3 encoders Huffman-code the quantized coefficients before archiving or transmission for
additional lossless compression. Bit streams range from 32 to 320 kbps, and 128-kbps rates
achieve near-CD quality, an important specification to enable dual-channel ISDN
(integrated-services-digital-network) to be the future high-bandwidth pipe to the home.

*/
static void DISABLE( AudioHLECommand command )
{
	//MessageBox (NULL, "Help", "ABI 3 Command 0", MB_OK);
	//ChangeABI (5);
}


AudioHLEInstruction ABI3[0x20] =
{
    DISABLE , ADPCM3 , CLEARBUFF3,	ENVMIXER3  , LOADBUFF3, RESAMPLE3  , SAVEBUFF3, MP3,
	MP3ADDY, SETVOL3, DMEMMOVE3 , LOADADPCM3 , MIXER3   , INTERLEAVE3, WHATISTHIS   , SETLOOP3,
    SPNOOP , SPNOOP, SPNOOP   , SPNOOP    , SPNOOP  , SPNOOP    , SPNOOP  , SPNOOP,
    SPNOOP , SPNOOP, SPNOOP   , SPNOOP    , SPNOOP  , SPNOOP    , SPNOOP  , SPNOOP
};
