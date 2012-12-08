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
#include "AudioHLEProcessor.h"

#include "audiohle.h"

#include "Math/MathUtil.h"
#include "Utility/FastMemcpy.h"

inline s32		FixedPointMulFull16( s32 a, s32 b )
{
	return s32( ( (s64)a * (s64)b ) >> 16 );
}

inline s32		FixedPointMul16( s32 a, s32 b )
{
	return s32( ( a * b ) >> 16 );
}

inline s32		FixedPointMul15( s32 a, s32 b )
{
	return s32( ( a * b ) >> 15 );
}

AudioHLEState gAudioHLEState;

void	AudioHLEState::ClearBuffer( u16 addr, u16 count )
{
	// XXXX check endianness
	memset( Buffer+(addr & (N64_AUDIO_BUFF - 4)), 0, (count+3) & (N64_AUDIO_BUFF - 4));
}

void	AudioHLEState::EnvMixer( u8 flags, u32 address )
{
	//static
	// ********* Make sure these conditions are met... ***********
	/*if ((InBuffer | OutBuffer | AuxA | AuxC | AuxE | Count) & 0x3) {
	MessageBox (NULL, "Unaligned EnvMixer... please report this to Azimer with the following information: RomTitle, Place in the rom it occurred, and any save state just before the error", "AudioHLE Error", MB_OK);
	}*/
	// ------------------------------------------------------------
	s16 *inp=(s16 *)(Buffer+InBuffer);
	s16 *out=(s16 *)(Buffer+OutBuffer);
	s16 *aux1=(s16 *)(Buffer+AuxA);
	s16 *aux2=(s16 *)(Buffer+AuxC);
	s16 *aux3=(s16 *)(Buffer+AuxE);
	s32 MainR;
	s32 MainL;
	s32 AuxR;
	s32 AuxL;
	s32 i1,o1,a1,a2=0,a3=0;
	u16 AuxIncRate=1;
	s16 zero[8];
	memset(zero,0,16);
	s32 LVol, RVol;
	s32 LAcc, RAcc;
	s32 LTrg, RTrg;
	s16 Wet, Dry;
	u32 ptr = 0;
	s32 RRamp, LRamp;
	s32 LAdderStart, RAdderStart, LAdderEnd, RAdderEnd;
	s32 oMainR, oMainL, oAuxR, oAuxL;

	//envmixcnt++;

	//fprintf (dfile, "\n----------------------------------------------------\n");
	if (flags & A_INIT)
	{
		LVol = ((VolLeft  * VolRampLeft));
		RVol = ((VolRight * VolRampRight));
		Wet = EnvWet;
		Dry = EnvDry; // Save Wet/Dry values
		LTrg = (VolTrgLeft << 16); RTrg = (VolTrgRight << 16); // Save Current Left/Right Targets
		LAdderStart = VolLeft  << 16;
		RAdderStart = VolRight << 16;
		LAdderEnd = LVol;
		RAdderEnd = RVol;
		RRamp = VolRampRight;
		LRamp = VolRampLeft;
	}
	else
	{
		// Load LVol, RVol, LAcc, and RAcc (all 32bit)
		// Load Wet, Dry, LTrg, RTrg
		memcpy((u8 *)MixerWorkArea, (rdram+address), 80);
		Wet			= *(s16 *)(MixerWorkArea +  0); // 0-1
		Dry			= *(s16 *)(MixerWorkArea +  2); // 2-3
		LTrg		= *(s32 *)(MixerWorkArea +  4); // 4-5
		RTrg		= *(s32 *)(MixerWorkArea +  6); // 6-7
		LRamp		= *(s32 *)(MixerWorkArea +  8); // 8-9 (MixerWorkArea is a 16bit pointer)
		RRamp		= *(s32 *)(MixerWorkArea + 10); // 10-11
		LAdderEnd	= *(s32 *)(MixerWorkArea + 12); // 12-13
		RAdderEnd	= *(s32 *)(MixerWorkArea + 14); // 14-15
		LAdderStart = *(s32 *)(MixerWorkArea + 16); // 12-13
		RAdderStart = *(s32 *)(MixerWorkArea + 18); // 14-15
	}

	if(!(flags&A_AUX))
	{
		AuxIncRate=0;
		aux2=aux3=zero;
	}

	oMainL = (Dry * (LTrg>>16) + 0x4000) >> 15;
	oAuxL  = (Wet * (LTrg>>16) + 0x4000) >> 15;
	oMainR = (Dry * (RTrg>>16) + 0x4000) >> 15;
	oAuxR  = (Wet * (RTrg>>16) + 0x4000) >> 15;

	for (s32 y = 0; y < Count; y += 0x10)
	{
		if (LAdderStart != LTrg)
		{
			//LAcc = LAdderStart;
			//LVol = (LAdderEnd - LAdderStart) >> 3;
			//LAdderEnd   = ((s64)LAdderEnd * (s64)LRamp) >> 16;
			//LAdderStart = ((s64)LAcc * (s64)LRamp) >> 16;

			// Assembly code which this replaces slightly different from commented out code above...
			u32 orig_ladder_end = LAdderEnd;
			LAcc = LAdderStart;
			LVol = (LAdderEnd - LAdderStart) >> 3;
			LAdderEnd = FixedPointMulFull16( LAdderEnd, LRamp );
			LAdderStart = orig_ladder_end;

		}
		else
		{
			LAcc = LTrg;
			LVol = 0;
		}

		if (RAdderStart != RTrg)
		{
			//RAcc = RAdderStart;
			//RVol = (RAdderEnd - RAdderStart) >> 3;
			//RAdderEnd   = ((s64)RAdderEnd * (s64)RRamp) >> 16;
			//RAdderStart = ((s64)RAcc * (s64)RRamp) >> 16;

			u32 orig_radder_end = RAdderEnd;
			RAcc = RAdderStart;
			RVol = (orig_radder_end - RAdderStart) >> 3;
			RAdderEnd = FixedPointMulFull16( RAdderEnd, RRamp );
			RAdderStart = orig_radder_end;
		}
		else
		{
			RAcc = RTrg;
			RVol = 0;
		}

		for (s32 x = 0; x < 8; x++)
		{
			i1=(s32)inp[ptr^1];
			o1=(s32)out[ptr^1];
			a1=(s32)aux1[ptr^1];
			if (AuxIncRate)
			{
				a2=(s32)aux2[ptr^1];
				a3=(s32)aux3[ptr^1];
			}
			// TODO: here...
			//LAcc = LTrg;
			//RAcc = RTrg;

			LAcc += LVol;
			RAcc += RVol;

			if (LVol <= 0)
			{
				// Decrementing
				if (LAcc < LTrg)
				{
					LAcc = LTrg;
					LAdderStart = LTrg;
					MainL = oMainL;
					AuxL  = oAuxL;
				}
				else
				{
					MainL = (Dry * ((s32)LAcc>>16) + 0x4000) >> 15;
					AuxL  = (Wet * ((s32)LAcc>>16) + 0x4000) >> 15;
				}
			}
			else
			{
				if (LAcc > LTrg)
				{
					LAcc = LTrg;
					LAdderStart = LTrg;
					MainL = oMainL;
					AuxL  = oAuxL;
				}
				else
				{
					MainL = (Dry * ((s32)LAcc>>16) + 0x4000) >> 15;
					AuxL  = (Wet * ((s32)LAcc>>16) + 0x4000) >> 15;
				}
			}

			if (RVol <= 0)
			{
				// Decrementing
				if (RAcc < RTrg)
				{
					RAcc = RTrg;
					RAdderStart = RTrg;
					MainR = oMainR;
					AuxR  = oAuxR;
				}
				else
				{
					MainR = (Dry * ((s32)RAcc>>16) + 0x4000) >> 15;
					AuxR  = (Wet * ((s32)RAcc>>16) + 0x4000) >> 15;
				}
			}
			else
			{
				if (RAcc > RTrg)
				{
					RAcc = RTrg;
					RAdderStart = RTrg;
					MainR = oMainR;
					AuxR  = oAuxR;
				}
				else
				{
					MainR = (Dry * ((s32)RAcc>>16) + 0x4000) >> 15;
					AuxR  = (Wet * ((s32)RAcc>>16) + 0x4000) >> 15;
				}
			}

			//fprintf (dfile, "%04X ", (LAcc>>16));

			o1+=(/*(o1*0x7fff)+*/(i1*MainR)+0x4000) >> 15;
			a1+=(/*(a1*0x7fff)+*/(i1*MainL)+0x4000) >> 15;

			/*		o1=((s64)(((s64)o1*0xfffe)+((s64)i1*MainR*2)+0x8000)>>16);

			a1=((s64)(((s64)a1*0xfffe)+((s64)i1*MainL*2)+0x8000)>>16);*/

			o1 = Saturate<s16>( o1 );
			a1 = Saturate<s16>( a1 );

			out[ptr^1]=o1;
			aux1[ptr^1]=a1;
			if (AuxIncRate)
			{
				//a2=((s64)(((s64)a2*0xfffe)+((s64)i1*AuxR*2)+0x8000)>>16);

				//a3=((s64)(((s64)a3*0xfffe)+((s64)i1*AuxL*2)+0x8000)>>16);
				a2+=(/*(a2*0x7fff)+*/(i1*AuxR)+0x4000)>>15;
				a3+=(/*(a3*0x7fff)+*/(i1*AuxL)+0x4000)>>15;

				a2 = Saturate<s16>( a2 );
				a3 = Saturate<s16>( a3 );

				aux2[ptr^1]=a2;
				aux3[ptr^1]=a3;
			}
			ptr++;
		}
	}

	/*LAcc = LAdderEnd;
	RAcc = RAdderEnd;*/

	*(s16 *)(MixerWorkArea +  0) = Wet; // 0-1
	*(s16 *)(MixerWorkArea +  2) = Dry; // 2-3
	*(s32 *)(MixerWorkArea +  4) = LTrg; // 4-5
	*(s32 *)(MixerWorkArea +  6) = RTrg; // 6-7
	*(s32 *)(MixerWorkArea +  8) = LRamp; // 8-9 (MixerWorkArea is a 16bit pointer)
	*(s32 *)(MixerWorkArea + 10) = RRamp; // 10-11
	*(s32 *)(MixerWorkArea + 12) = LAdderEnd; // 12-13
	*(s32 *)(MixerWorkArea + 14) = RAdderEnd; // 14-15
	*(s32 *)(MixerWorkArea + 16) = LAdderStart; // 12-13
	*(s32 *)(MixerWorkArea + 18) = RAdderStart; // 14-15
	memcpy(rdram+address, (u8 *)MixerWorkArea,80);
}

#if 1 //1->fast, 0->original Azimer //Corn calc two sample (s16) at once so we get to save a u32
void	AudioHLEState::Resample( u8 flags, u32 pitch, u32 address )
{
	DAEDALUS_ASSERT( (flags & 0x2) == 0, "Resample: unhandled flags %02x", flags );		// Was breakpoint - StrmnNrmn

	pitch *= 2;

	s16 *	in ( (s16 *)(Buffer) );
	u32 *	out( (u32 *)(Buffer) );	//Save some bandwith and fuse two sample in one write
	u32		srcPtr((InBuffer / 2) - 1);
	u32		dstPtr(OutBuffer / 4);
	u32		tmp;

	u32 accumulator;
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

	for(u32 i = (((Count + 0xF) & (N64_AUDIO_BUFF - 0x10)) >> 2); i != 0 ; i-- )
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
}

#else

//Needed for Azimers resample alghorithm
const u16 ResampleLUT[0x200] =
{
	0x0C39, 0x66AD, 0x0D46, 0xFFDF, 0x0B39, 0x6696, 0x0E5F, 0xFFD8,
	0x0A44, 0x6669, 0x0F83, 0xFFD0, 0x095A, 0x6626, 0x10B4, 0xFFC8,
	0x087D, 0x65CD, 0x11F0, 0xFFBF, 0x07AB, 0x655E, 0x1338, 0xFFB6,
	0x06E4, 0x64D9, 0x148C, 0xFFAC, 0x0628, 0x643F, 0x15EB, 0xFFA1,
	0x0577, 0x638F, 0x1756, 0xFF96, 0x04D1, 0x62CB, 0x18CB, 0xFF8A,
	0x0435, 0x61F3, 0x1A4C, 0xFF7E, 0x03A4, 0x6106, 0x1BD7, 0xFF71,
	0x031C, 0x6007, 0x1D6C, 0xFF64, 0x029F, 0x5EF5, 0x1F0B, 0xFF56,
	0x022A, 0x5DD0, 0x20B3, 0xFF48, 0x01BE, 0x5C9A, 0x2264, 0xFF3A,
	0x015B, 0x5B53, 0x241E, 0xFF2C, 0x0101, 0x59FC, 0x25E0, 0xFF1E,
	0x00AE, 0x5896, 0x27A9, 0xFF10, 0x0063, 0x5720, 0x297A, 0xFF02,
	0x001F, 0x559D, 0x2B50, 0xFEF4, 0xFFE2, 0x540D, 0x2D2C, 0xFEE8,
	0xFFAC, 0x5270, 0x2F0D, 0xFEDB, 0xFF7C, 0x50C7, 0x30F3, 0xFED0,
	0xFF53, 0x4F14, 0x32DC, 0xFEC6, 0xFF2E, 0x4D57, 0x34C8, 0xFEBD,
	0xFF0F, 0x4B91, 0x36B6, 0xFEB6, 0xFEF5, 0x49C2, 0x38A5, 0xFEB0,
	0xFEDF, 0x47ED, 0x3A95, 0xFEAC, 0xFECE, 0x4611, 0x3C85, 0xFEAB,
	0xFEC0, 0x4430, 0x3E74, 0xFEAC, 0xFEB6, 0x424A, 0x4060, 0xFEAF,
	0xFEAF, 0x4060, 0x424A, 0xFEB6, 0xFEAC, 0x3E74, 0x4430, 0xFEC0,
	0xFEAB, 0x3C85, 0x4611, 0xFECE, 0xFEAC, 0x3A95, 0x47ED, 0xFEDF,
	0xFEB0, 0x38A5, 0x49C2, 0xFEF5, 0xFEB6, 0x36B6, 0x4B91, 0xFF0F,
	0xFEBD, 0x34C8, 0x4D57, 0xFF2E, 0xFEC6, 0x32DC, 0x4F14, 0xFF53,
	0xFED0, 0x30F3, 0x50C7, 0xFF7C, 0xFEDB, 0x2F0D, 0x5270, 0xFFAC,
	0xFEE8, 0x2D2C, 0x540D, 0xFFE2, 0xFEF4, 0x2B50, 0x559D, 0x001F,
	0xFF02, 0x297A, 0x5720, 0x0063, 0xFF10, 0x27A9, 0x5896, 0x00AE,
	0xFF1E, 0x25E0, 0x59FC, 0x0101, 0xFF2C, 0x241E, 0x5B53, 0x015B,
	0xFF3A, 0x2264, 0x5C9A, 0x01BE, 0xFF48, 0x20B3, 0x5DD0, 0x022A,
	0xFF56, 0x1F0B, 0x5EF5, 0x029F, 0xFF64, 0x1D6C, 0x6007, 0x031C,
	0xFF71, 0x1BD7, 0x6106, 0x03A4, 0xFF7E, 0x1A4C, 0x61F3, 0x0435,
	0xFF8A, 0x18CB, 0x62CB, 0x04D1, 0xFF96, 0x1756, 0x638F, 0x0577,
	0xFFA1, 0x15EB, 0x643F, 0x0628, 0xFFAC, 0x148C, 0x64D9, 0x06E4,
	0xFFB6, 0x1338, 0x655E, 0x07AB, 0xFFBF, 0x11F0, 0x65CD, 0x087D,
	0xFFC8, 0x10B4, 0x6626, 0x095A, 0xFFD0, 0x0F83, 0x6669, 0x0A44,
	0xFFD8, 0x0E5F, 0x6696, 0x0B39, 0xFFDF, 0x0D46, 0x66AD, 0x0C39
};

void	AudioHLEState::Resample( u8 flags, u32 pitch, u32 address )
{
	bool	init( (flags & 0x1) != 0 );
	DAEDALUS_ASSERT( (flags & 0x2) == 0, "Resample: unhandled flags %02x", flags );		// Was breakpoint - StrmnNrmn

	pitch *= 2;

	s16 *	buffer( (s16 *)(Buffer) );
	u32		srcPtr(InBuffer/2);
	u32		dstPtr(OutBuffer/2);
	srcPtr -= 4;

	u32 accumulator;
	if (init)
	{
		for (u32 x=0; x < 4; x++)
		{
			buffer[(srcPtr+x)^1] = 0;
		}
		accumulator = 0;
	}
	else
	{
		for (u32 x=0; x < 4; x++)
		{
			buffer[(srcPtr+x)^1] = ((u16 *)rdram)[((address/2)+x)^1];
		}
		accumulator = *(u16 *)(rdram+address+10);
	}


	u32		loops( ((Count+0xf) & (N64_AUDIO_BUFF - 0x10))/2 );
	for(u32 i = 0; i < loops ; ++i )
	{
		u32			location( (accumulator >> 0xa) << 0x3 );
		const s16 *	lut( (s16 *)(((u8 *)ResampleLUT) + location) );

		s32 accum;

		accum  = FixedPointMul15( buffer[(srcPtr+0)^1], lut[0] );
		accum += FixedPointMul15( buffer[(srcPtr+1)^1], lut[1] );
		accum += FixedPointMul15( buffer[(srcPtr+2)^1], lut[2] );
		accum += FixedPointMul15( buffer[(srcPtr+3)^1], lut[3] );

		buffer[dstPtr^1] = Saturate<s16>(accum);
		dstPtr++;
		accumulator += pitch;
		srcPtr += (accumulator>>16);
		accumulator&=0xffff;
	}

	for (u32 x=0; x < 4; x++)
	{
		((u16 *)rdram)[((address/2)+x)^1] = buffer[(srcPtr+x)^1];
	}
	*(u16 *)(rdram+address+10) = (u16)accumulator;
}
#endif

inline void AudioHLEState::ExtractSamplesScale( s32 * output, u32 inPtr, s32 vscale ) const
{
	u8 icode;

	// loop of 8, for 8 coded nibbles from 4 bytes which yields 8 s16 pcm values
	icode = Buffer[(InBuffer+inPtr++)^3];
	*output++ = FixedPointMul16( (s16)((icode&0xf0)<< 8), vscale );
	*output++ = FixedPointMul16( (s16)((icode&0x0f)<<12), vscale );
	icode = Buffer[(InBuffer+inPtr++)^3];
	*output++ = FixedPointMul16( (s16)((icode&0xf0)<< 8), vscale );
	*output++ = FixedPointMul16( (s16)((icode&0x0f)<<12), vscale );
	icode = Buffer[(InBuffer+inPtr++)^3];
	*output++ = FixedPointMul16( (s16)((icode&0xf0)<< 8), vscale );
	*output++ = FixedPointMul16( (s16)((icode&0x0f)<<12), vscale );
	icode = Buffer[(InBuffer+inPtr++)^3];
	*output++ = FixedPointMul16( (s16)((icode&0xf0)<< 8), vscale );
	*output++ = FixedPointMul16( (s16)((icode&0x0f)<<12), vscale );
}

inline void AudioHLEState::ExtractSamples( s32 * output, u32 inPtr ) const
{
	u8 icode;

	// loop of 8, for 8 coded nibbles from 4 bytes which yields 8 s16 pcm values
	icode = Buffer[(InBuffer+inPtr++)^3];
	*output++ = (s16)((icode&0xf0)<< 8);
	*output++ = (s16)((icode&0x0f)<<12);
	icode = Buffer[(InBuffer+inPtr++)^3];
	*output++ = (s16)((icode&0xf0)<< 8);
	*output++ = (s16)((icode&0x0f)<<12);
	icode = Buffer[(InBuffer+inPtr++)^3];
	*output++ = (s16)((icode&0xf0)<< 8);
	*output++ = (s16)((icode&0x0f)<<12);
	icode = Buffer[(InBuffer+inPtr++)^3];
	*output++ = (s16)((icode&0xf0)<< 8);
	*output++ = (s16)((icode&0x0f)<<12);
}

//
//	l1/l2 are IN/OUT
//
#if 1 //1->fast, 0->original Azimer //Corn
inline void DecodeSamples( s16 * out, s32 & l1, s32 & l2, const s32 * input, const s16 * book1, const s16 * book2 )
{
	s32 a[8];

	a[0]= (s32)book1[0]*l1;
	a[0]+=(s32)book2[0]*l2;
	a[0]+=input[0]*2048;

	a[1] =(s32)book1[1]*l1;
	a[1]+=(s32)book2[1]*l2;
	a[1]+=(s32)book2[0]*input[0];
	a[1]+=input[1]*2048;

	a[2] =(s32)book1[2]*l1;
	a[2]+=(s32)book2[2]*l2;
	a[2]+=(s32)book2[1]*input[0];
	a[2]+=(s32)book2[0]*input[1];
	a[2]+=input[2]*2048;

	a[3] =(s32)book1[3]*l1;
	a[3]+=(s32)book2[3]*l2;
	a[3]+=(s32)book2[2]*input[0];
	a[3]+=(s32)book2[1]*input[1];
	a[3]+=(s32)book2[0]*input[2];
	a[3]+=input[3]*2048;

	a[4] =(s32)book1[4]*l1;
	a[4]+=(s32)book2[4]*l2;
	a[4]+=(s32)book2[3]*input[0];
	a[4]+=(s32)book2[2]*input[1];
	a[4]+=(s32)book2[1]*input[2];
	a[4]+=(s32)book2[0]*input[3];
	a[4]+=input[4]*2048;

	a[5] =(s32)book1[5]*l1;
	a[5]+=(s32)book2[5]*l2;
	a[5]+=(s32)book2[4]*input[0];
	a[5]+=(s32)book2[3]*input[1];
	a[5]+=(s32)book2[2]*input[2];
	a[5]+=(s32)book2[1]*input[3];
	a[5]+=(s32)book2[0]*input[4];
	a[5]+=input[5]*2048;

	a[6] =(s32)book1[6]*l1;
	a[6]+=(s32)book2[6]*l2;
	a[6]+=(s32)book2[5]*input[0];
	a[6]+=(s32)book2[4]*input[1];
	a[6]+=(s32)book2[3]*input[2];
	a[6]+=(s32)book2[2]*input[3];
	a[6]+=(s32)book2[1]*input[4];
	a[6]+=(s32)book2[0]*input[5];
	a[6]+=input[6]*2048;

	a[7] =(s32)book1[7]*l1;
	a[7]+=(s32)book2[7]*l2;
	a[7]+=(s32)book2[6]*input[0];
	a[7]+=(s32)book2[5]*input[1];
	a[7]+=(s32)book2[4]*input[2];
	a[7]+=(s32)book2[3]*input[3];
	a[7]+=(s32)book2[2]*input[4];
	a[7]+=(s32)book2[1]*input[5];
	a[7]+=(s32)book2[0]*input[6];
	a[7]+=input[7]*2048;

	*out++ =      Saturate<s16>( a[1] >> 11 );
	*out++ =      Saturate<s16>( a[0] >> 11 );
	*out++ =      Saturate<s16>( a[3] >> 11 );
	*out++ =      Saturate<s16>( a[2] >> 11 );
	*out++ =      Saturate<s16>( a[5] >> 11 );
	*out++ =      Saturate<s16>( a[4] >> 11 );
	*out++ = l2 = Saturate<s16>( a[7] >> 11 );
	*out++ = l1 = Saturate<s16>( a[6] >> 11 );
}

#else
inline void DecodeSamples( s16 * out, s32 & l1, s32 & l2, const s32 * input, const s16 * book1, const s16 * book2 )
{
	s32 a[8];

	a[0]= (s32)book1[0]*l1;
	a[0]+=(s32)book2[0]*l2;
	a[0]+=input[0]*2048;

	a[1] =(s32)book1[1]*l1;
	a[1]+=(s32)book2[1]*l2;
	a[1]+=(s32)book2[0]*input[0];
	a[1]+=input[1]*2048;

	a[2] =(s32)book1[2]*l1;
	a[2]+=(s32)book2[2]*l2;
	a[2]+=(s32)book2[1]*input[0];
	a[2]+=(s32)book2[0]*input[1];
	a[2]+=input[2]*2048;

	a[3] =(s32)book1[3]*l1;
	a[3]+=(s32)book2[3]*l2;
	a[3]+=(s32)book2[2]*input[0];
	a[3]+=(s32)book2[1]*input[1];
	a[3]+=(s32)book2[0]*input[2];
	a[3]+=input[3]*2048;

	a[4] =(s32)book1[4]*l1;
	a[4]+=(s32)book2[4]*l2;
	a[4]+=(s32)book2[3]*input[0];
	a[4]+=(s32)book2[2]*input[1];
	a[4]+=(s32)book2[1]*input[2];
	a[4]+=(s32)book2[0]*input[3];
	a[4]+=input[4]*2048;

	a[5] =(s32)book1[5]*l1;
	a[5]+=(s32)book2[5]*l2;
	a[5]+=(s32)book2[4]*input[0];
	a[5]+=(s32)book2[3]*input[1];
	a[5]+=(s32)book2[2]*input[2];
	a[5]+=(s32)book2[1]*input[3];
	a[5]+=(s32)book2[0]*input[4];
	a[5]+=input[5]*2048;

	a[6] =(s32)book1[6]*l1;
	a[6]+=(s32)book2[6]*l2;
	a[6]+=(s32)book2[5]*input[0];
	a[6]+=(s32)book2[4]*input[1];
	a[6]+=(s32)book2[3]*input[2];
	a[6]+=(s32)book2[2]*input[3];
	a[6]+=(s32)book2[1]*input[4];
	a[6]+=(s32)book2[0]*input[5];
	a[6]+=input[6]*2048;

	a[7] =(s32)book1[7]*l1;
	a[7]+=(s32)book2[7]*l2;
	a[7]+=(s32)book2[6]*input[0];
	a[7]+=(s32)book2[5]*input[1];
	a[7]+=(s32)book2[4]*input[2];
	a[7]+=(s32)book2[3]*input[3];
	a[7]+=(s32)book2[2]*input[4];
	a[7]+=(s32)book2[1]*input[5];
	a[7]+=(s32)book2[0]*input[6];
	a[7]+=input[7]*2048;

	s16 r[8];
	for(u32 j=0;j<8;j++)
	{
		u32 idx( j^1 );
		r[idx] = Saturate<s16>( a[idx] >> 11 );
		*(out++) = r[idx];
	}

	l1=r[6];
	l2=r[7];
}
#endif

void AudioHLEState::ADPCMDecode( u8 flags, u32 address )
{
	bool	init( (flags&0x1) != 0 );
	bool	loop( (flags&0x2) != 0 );

	u16 inPtr=0;
	s16 *out=(s16 *)(Buffer+OutBuffer);

	if(init)
	{
		memset( out, 0, 32 );
	}
	else
	{
		u32 addr( loop ? (LoopVal&0x7fffff) : address );
		memcpy( out, &rdram[addr], 32 );
	}

	s32 l1=out[15];
	s32 l2=out[14];
	out+=16;

	s32 inp1[8];
	s32 inp2[8];

	s32 count = (s16)Count;		// XXXX why convert this to signed?
	while(count>0)
	{
													// the first iteration through, these values are
													// either 0 in the case of A_INIT, from a special
													// area of memory in the case of A_LOOP or just
													// the values we calculated the last time

		u8 code=Buffer[(InBuffer+inPtr)^3];
		u32 index=code&0xf;							// index into the adpcm code table
		s16 * book1=(s16 *)&ADPCMTable[index<<4];
		s16 * book2=book1+8;
		code>>=4;									// upper nibble is scale

		inPtr++;									// coded adpcm data lies next

		if( code < 12 )
		{
			s32 vscale=(0x8000>>((12-code)-1));			// very strange. 0x8000 would be .5 in 16:16 format
														// so this appears to be a fractional scale based
														// on the 12 based inverse of the scale value.  note
														// that this could be negative, in which case we do
														// not use the calculated vscale value... see the
														// if(code>12) check below
			ExtractSamplesScale( inp1, inPtr + 0, vscale );
			ExtractSamplesScale( inp2, inPtr + 4, vscale );
		}
		else
		{
			ExtractSamples( inp1, inPtr + 0 );
			ExtractSamples( inp2, inPtr + 4 );
		}

		DecodeSamples( out + 0, l1, l2, inp1, book1, book2 );
		DecodeSamples( out + 8, l1, l2, inp2, book1, book2 );

		inPtr += 8;
		out += 16;
		count-=32;
	}
	out-=16;
	memcpy(&rdram[address],out,32);
}

void	AudioHLEState::LoadBuffer( u32 address )
{
	LoadBuffer( InBuffer, address, Count );
}

void	AudioHLEState::SaveBuffer( u32 address )
{
	SaveBuffer( address, OutBuffer, Count );
}

void	AudioHLEState::LoadBuffer( u16 dram_dst, u32 ram_src, u16 count )
{
	if( count > 0 )
	{
		// XXXX Masks look suspicious - trying to get around endian issues?
		memcpy( Buffer+(dram_dst & (N64_AUDIO_BUFF - 4)), rdram+(ram_src&0xfffffc), (count+3) & (N64_AUDIO_BUFF - 4) );
	}
}


void	AudioHLEState::SaveBuffer( u32 ram_dst, u16 dmem_src, u16 count )
{
	if( count > 0 )
	{
		// XXXX Masks look suspicious - trying to get around endian issues?
		memcpy( rdram+(ram_dst & 0xfffffc), Buffer+(dmem_src & (N64_AUDIO_BUFF - 4)), (count+3) & (N64_AUDIO_BUFF - 4));
	}
}
/*
void	AudioHLEState::SetSegment( u8 segment, u32 address )
{
	DAEDALUS_ASSERT( segment < 16, "Invalid segment" );

	Segments[segment&0xf] = address;
}
*/
void	AudioHLEState::SetLoop( u32 loopval )
{
	LoopVal = loopval;
	//VolTrgLeft  = (s16)(LoopVal>>16);		// m_LeftVol
	//VolRampLeft = (s16)(LoopVal);	// m_LeftVolTarget
}


void	AudioHLEState::SetBuffer( u8 flags, u16 in, u16 out, u16 count )
{
	if (flags & 0x8)
	{
		// A_AUX - Auxillary Sound Buffer Settings
		AuxA = in;
		AuxC = out;
		AuxE = count;
	}
	else
	{
		// A_MAIN - Main Sound Buffer Settings
		InBuffer  = in;
		OutBuffer = out;
		Count	  = count;
	}
}

void	AudioHLEState::DmemMove( u16 dst, u16 src, u16 count )
{
	memcpy_cpu_LE(Buffer + dst, Buffer + src, (count + 3) & (N64_AUDIO_BUFF - 4));

	/*count = (count + 3) & (N64_AUDIO_BUFF - 4);
	for (u32 i = 0; i < count; i++)
	{
		*(u8 *)(Buffer+((i+dst)^3)) = *(u8 *)(Buffer+((i+src)^3));
	}*/
}

void	AudioHLEState::LoadADPCM( u32 address, u16 count )
{
	u32	loops( count / 16 );

	const u16 *table( (const u16 *)(rdram + address) );
	for (u32 x = 0; x < loops; x++)
	{
		ADPCMTable[0x1+(x<<3)] = table[0];
		ADPCMTable[0x0+(x<<3)] = table[1];

		ADPCMTable[0x3+(x<<3)] = table[2];
		ADPCMTable[0x2+(x<<3)] = table[3];

		ADPCMTable[0x5+(x<<3)] = table[4];
		ADPCMTable[0x4+(x<<3)] = table[5];

		ADPCMTable[0x7+(x<<3)] = table[6];
		ADPCMTable[0x6+(x<<3)] = table[7];
		table += 8;
	}
}

void	AudioHLEState::Interleave( u16 outaddr, u16 laddr, u16 raddr, u16 count )
{
	u32 *		out = (u32 *)(Buffer + outaddr);	//Save some bandwith also corrected left and right//Corn
	const u16 *	inr = (const u16 *)(Buffer + raddr);
	const u16 *	inl = (const u16 *)(Buffer + laddr);

	for( u32 x = (count >> 2); x != 0; x-- )
	{
		const u16 right = *inr++;
		const u16 left  = *inl++;

		*out++ = (*inr++ << 16) | *inl++;
		*out++ = (right  << 16) | left;
	}
}

void	AudioHLEState::Interleave( u16 laddr, u16 raddr )
{
	Interleave( OutBuffer, laddr, raddr, Count );
}

void	AudioHLEState::Mixer( u16 dmemout, u16 dmemin, s32 gain, u16 count )
{
#if 1	//1->fast, 0->safe/slow //Corn
	// Check if outside of buffer (YOSHI)
	if( ((u32)dmemin + (count >> 1)) & ~(N64_AUDIO_BUFF - 1) ) return;

	// Make sure we are on even address (YOSHI)
	s16*  in( (s16 *)(Buffer + (dmemin  & (N64_AUDIO_BUFF - 2))) );
	s16* out( (s16 *)(Buffer + (dmemout & (N64_AUDIO_BUFF - 2))) );

	for( u32 x = count >> 1; x != 0; x-- )
	{
		*out = Saturate<s16>( FixedPointMul15( *in++, gain ) + s32( *out ) );
		out++;
	}

#else
	for( u32 x=0; x < count; x+=2 )
	{
		s16 in( *(s16 *)(Buffer+((dmemin+x) & (N64_AUDIO_BUFF - 2))) );
		s16 out( *(s16 *)(Buffer+((dmemout+x) & (N64_AUDIO_BUFF - 2))) );

		*(s16 *)(Buffer+((dmemout+x) & (N64_AUDIO_BUFF - 2)) ) = Saturate<s16>( FixedPointMul15( in, gain ) + s32( out ) );
	}
#endif
}

void	AudioHLEState::Deinterleave( u16 outaddr, u16 inaddr, u16 count )
{
	while( count-- )
	{
		*(s16 *)(Buffer+(outaddr^2)) = *(s16 *)(Buffer+(inaddr^2));
		outaddr += 2;
		inaddr  += 4;
	}
}

void	AudioHLEState::Mixer( u16 dmemout, u16 dmemin, s32 gain )
{
	Mixer( dmemout, dmemin, gain, Count );
}
