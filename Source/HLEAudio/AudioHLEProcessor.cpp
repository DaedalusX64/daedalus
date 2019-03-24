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

#include <string.h>

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
	memset( Buffer+(addr & 0xfffc), 0, (count+3) & 0xfffc );
}

void	AudioHLEState::EnvMixer( u8 flags, u32 address )
{
	//static
	// ********* Make sure these conditions are met... ***********
	/*if ((InBuffer | OutBuffer | AuxA | AuxC | AuxE | Count) & 0x3) {
	MessageBox (NULL, "Unaligned EnvMixer... please report this to Azimer with the following information: RomTitle, Place in the rom it occurred, and any save state just before the error", "AudioHLE Error", MB_OK);
	}*/
	// ------------------------------------------------------------
	s16 *inp {(s16 *)(Buffer+InBuffer)};
	s16 *out {(s16 *)(Buffer+OutBuffer)};
	s16 *aux1 {(s16 *)(Buffer+AuxA)};
	s16 *aux2 {(s16 *)(Buffer+AuxC)};
	s16 *aux3 {(s16 *)(Buffer+AuxE)};
	s32 MainR {}, MainL {}, AuxR {}, AuxL {};

	s32 i1 {},o1 {},a1 {},a2 {},a3 {};
	u16 AuxIncRate{1};
	s16 zero[8] {};
	memset(zero,0,16);
	s32 LVol {}, RVol {};
	s32 LAcc {}, RAcc {};
	s32 LTrg {}, RTrg {};
	s16 Wet {}, Dry {};
	u32 ptr {};
	s32 RRamp {}, LRamp {};
	s32 LAdderStart{} , RAdderStart {}, LAdderEnd {}, RAdderEnd {};
	s32 oMainR {}, oMainL {}, oAuxR {}, oAuxL {};

	s16* buff {(s16*)(rdram+address)};

	//envmixcnt++;

	//fprintf (dfile, "\n----------------------------------------------------\n");
	if (flags & A_INIT)
	{
		LVol = ((VolLeft  * VolRampLeft));
		RVol = ((VolRight * VolRampRight));
		Wet = EnvWet;
		Dry = EnvDry; // Save Wet/Dry values
		LTrg = (VolTrgLeft << 16);
		RTrg = (VolTrgRight << 16); // Save Current Left/Right Targets
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
		Wet			= *(s16 *)(buff +  0); // 0-1
		Dry			= *(s16 *)(buff +  2); // 2-3
		LTrg		= *(s32 *)(buff +  4); // 4-5
		RTrg		= *(s32 *)(buff +  6); // 6-7
		LRamp		= *(s32 *)(buff +  8); // 8-9 (MixerWorkArea is a 16bit pointer)
		RRamp		= *(s32 *)(buff + 10); // 10-11
		LAdderEnd	= *(s32 *)(buff + 12); // 12-13
		RAdderEnd	= *(s32 *)(buff + 14); // 14-15
		LAdderStart = *(s32 *)(buff + 16); // 12-13
		RAdderStart = *(s32 *)(buff + 18); // 14-15
	}

	if(!(flags&A_AUX))
	{
		AuxIncRate = 0;
		aux2 = aux3 = zero;
	}

	oMainL = (Dry * (LTrg>>16) + 0x4000) >> 15;
	oAuxL  = (Wet * (LTrg>>16) + 0x4000) >> 15;
	oMainR = (Dry * (RTrg>>16) + 0x4000) >> 15;
	oAuxR  = (Wet * (RTrg>>16) + 0x4000) >> 15;

	for (s32 y {}; y < Count; y += 0x10)
	{
		if (LAdderStart != LTrg)
		{
			//LAcc = LAdderStart;
			//LVol = (LAdderEnd - LAdderStart) >> 3;
			//LAdderEnd   = ((s64)LAdderEnd * (s64)LRamp) >> 16;
			//LAdderStart = ((s64)LAcc * (s64)LRamp) >> 16;

			// Assembly code which this replaces slightly different from commented out code above...
			u32 orig_ladder_end {(u32)LAdderEnd};
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

			u32 orig_radder_end {(u32)RAdderEnd};
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

		for (s32 x {}; x < 8; x++)
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

			o1 += (/*(o1*0x7fff)+*/(i1*MainR) + 0x4000) >> 15;
			a1 += (/*(a1*0x7fff)+*/ (i1*MainL) + 0x4000) >> 15;

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

	*(s16 *)(buff +  0) = Wet; // 0-1
	*(s16 *)(buff +  2) = Dry; // 2-3
	*(s32 *)(buff +  4) = LTrg; // 4-5
	*(s32 *)(buff +  6) = RTrg; // 6-7
	*(s32 *)(buff +  8) = LRamp; // 8-9 (MixerWorkArea is a 16bit pointer)
	*(s32 *)(buff + 10) = RRamp; // 10-11
	*(s32 *)(buff + 12) = LAdderEnd; // 12-13
	*(s32 *)(buff + 14) = RAdderEnd; // 14-15
	*(s32 *)(buff + 16) = LAdderStart; // 12-13
	*(s32 *)(buff + 18) = RAdderStart; // 14-15
}

void	AudioHLEState::Resample( u8 flags, u32 pitch, u32 address )
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( (flags & 0x2) == 0, "Resample: unhandled flags %02x", flags );		// Was breakpoint - StrmnNrmn
	#endif
	pitch *= 2;

	s16 *	in ( (s16 *)(Buffer) );
	u32 *	out( (u32 *)(Buffer) );	//Save some bandwith and fuse two sample in one write
	u32		srcPtr((InBuffer / 2) - 1);
	u32		dstPtr(OutBuffer / 4);

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

	for(u32 i = (((Count + 0xF) & 0xFFF0) >> 2); i != 0 ; i-- )
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

inline void AudioHLEState::ExtractSamplesScale( s32 * output, u32 inPtr, s32 vscale ) const
{
	// loop of 8, for 8 coded nibbles from 4 bytes which yields 8 s16 pcm values
	u8 icode {Buffer[(InBuffer+inPtr++)^3]};
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
	// loop of 8, for 8 coded nibbles from 4 bytes which yields 8 s16 pcm values
	u8 icode {Buffer[(InBuffer+inPtr++)^3]};
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
inline void DecodeSamples( s16 * out, s32 & l1, s32 & l2, const s32 * input, const s16 * book1, const s16 * book2 )
{
	s32 a[8] {};

	a[0] = (s32)book1[0]*l1;
	a[0] += (s32)book2[0]*l2;
	a[0] += input[0]*2048;

	a[1] = (s32)book1[1]*l1;
	a[1] += (s32)book2[1]*l2;
	a[1] += (s32)book2[0]*input[0];
	a[1] += input[1]*2048;

	a[2] = (s32)book1[2]*l1;
	a[2] += (s32)book2[2]*l2;
	a[2] += (s32)book2[1]*input[0];
	a[2] += (s32)book2[0]*input[1];
	a[2] += input[2]*2048;

	a[3] = (s32)book1[3]*l1;
	a[3] += (s32)book2[3]*l2;
	a[3] += (s32)book2[2]*input[0];
	a[3] += (s32)book2[1]*input[1];
	a[3] += (s32)book2[0]*input[2];
	a[3] += input[3]*2048;

	a[4] = (s32)book1[4]*l1;
	a[4] += (s32)book2[4]*l2;
	a[4] += (s32)book2[3]*input[0];
	a[4] += (s32)book2[2]*input[1];
	a[4] += (s32)book2[1]*input[2];
	a[4] += (s32)book2[0]*input[3];
	a[4] += input[4]*2048;

	a[5] = (s32)book1[5]*l1;
	a[5] += (s32)book2[5]*l2;
	a[5] += (s32)book2[4]*input[0];
	a[5] += (s32)book2[3]*input[1];
	a[5] += (s32)book2[2]*input[2];
	a[5] += (s32)book2[1]*input[3];
	a[5] += (s32)book2[0]*input[4];
	a[5] += input[5]*2048;

	a[6] = (s32)book1[6]*l1;
	a[6] += (s32)book2[6]*l2;
	a[6] += (s32)book2[5]*input[0];
	a[6] += (s32)book2[4]*input[1];
	a[6] += (s32)book2[3]*input[2];
	a[6] += (s32)book2[2]*input[3];
	a[6] += (s32)book2[1]*input[4];
	a[6] += (s32)book2[0]*input[5];
	a[6] += input[6]*2048;

	a[7] = (s32)book1[7]*l1;
	a[7] += (s32)book2[7]*l2;
	a[7] += (s32)book2[6]*input[0];
	a[7] += (s32)book2[5]*input[1];
	a[7] += (s32)book2[4]*input[2];
	a[7] += (s32)book2[3]*input[3];
	a[7] += (s32)book2[2]*input[4];
	a[7] += (s32)book2[1]*input[5];
	a[7] += (s32)book2[0]*input[6];
	a[7] += input[7]*2048;

	*out++ =      Saturate<s16>( a[1] >> 11 );
	*out++ =      Saturate<s16>( a[0] >> 11 );
	*out++ =      Saturate<s16>( a[3] >> 11 );
	*out++ =      Saturate<s16>( a[2] >> 11 );
	*out++ =      Saturate<s16>( a[5] >> 11 );
	*out++ =      Saturate<s16>( a[4] >> 11 );
	*out++ = l2 = Saturate<s16>( a[7] >> 11 );
	*out++ = l1 = Saturate<s16>( a[6] >> 11 );
}




void AudioHLEState::ADPCMDecode( u8 flags, u32 address )
{
	bool	init( (flags&0x1) != 0 );
	bool	loop( (flags&0x2) != 0 );

	u16 inPtr {};
	s16 *out {(s16 *)(Buffer+OutBuffer)};

	if(init)
	{
		memset( out, 0, 32 );
	}
	else
	{
		u32 addr( loop ? LoopVal : address );
		memcpy( out, &rdram[addr], 32 );
	}

	s32 l1 {out[15]};
	s32 l2 {out[14]};
	out+=16;

	s32 inp1[8] {};
	s32 inp2[8] {};

	s32 count {(s16)Count};		// XXXX why convert this to signed?
	while(count>0)
	{
													// the first iteration through, these values are
													// either 0 in the case of A_INIT, from a special
													// area of memory in the case of A_LOOP or just
													// the values we calculated the last time

		u8 code {Buffer[(InBuffer+inPtr)^3]};
		u32 index {(u32)code&0xf};							// index into the adpcm code table
		s16 * book1 {(s16 *)&ADPCMTable[index<<4]};
		s16 * book2 {book1+8};
		code >>= 4 ;									// upper nibble is scale

		inPtr++;									// coded adpcm data lies next

		if( code < 12 )
		{
			s32 vscale {(0x8000>>((12-code)-1))};			// very strange. 0x8000 would be .5 in 16:16 format
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
		memcpy( Buffer+(dram_dst & 0xFFFC), rdram+(ram_src&0xfffffc), (count+3) & 0xFFFC );
	}
}


void	AudioHLEState::SaveBuffer( u32 ram_dst, u16 dmem_src, u16 count )
{
	if( count > 0 )
	{
		// XXXX Masks look suspicious - trying to get around endian issues?
		memcpy( rdram+(ram_dst & 0xfffffc), Buffer+(dmem_src & 0xFFFC), (count+3) & 0xFFFC);
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

void	AudioHLEState::DmemMove( u32 dst, u32 src, u16 count )
{
	count = (count + 3) & 0xfffc;

#if 1	//1->fast, 0->slow

	//Can't use fast_memcpy_swizzle, since this code can run on the ME, and VFPU is not accessible
	memcpy_swizzle(Buffer + dst, Buffer + src, count);
#else
	for (u32 i = 0; i < count; i++)
	{
		*(u8 *)(Buffer+((i+dst)^3)) = *(u8 *)(Buffer+((i+src)^3));
	}
#endif
}

void	AudioHLEState::LoadADPCM( u32 address, u16 count )
{
	u32	loops( count / 16 );

	const u16 *table( (const u16 *)(rdram + address) );
	for (u32 x {}; x < loops; x++)
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
	u32 *		out  {(u32 *)(Buffer + outaddr)};	//Save some bandwith also corrected left and right//Corn
	const u16 *	inr {(const u16 *)(Buffer + raddr)};
	const u16 *	inl {(const u16 *)(Buffer + laddr)};

	for( u32 x = (count >> 2); x != 0; x-- )
	{
		const u16 right {*inr++};
		const u16 left  {*inl++};

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

	// Make sure we are on even address (YOSHI)
	s16*  in( (s16 *)(Buffer + dmemin) );
	s16* out( (s16 *)(Buffer + dmemout) );

	for( u32 x = count >> 1; x != 0; x-- )
	{
		*out = Saturate<s16>( FixedPointMul15( *in++, gain ) + s32( *out ) );
		out++;
	}

#else
	for( u32 x {}; x < count; x+=2 )
	{
		s16 in( *(s16 *)(Buffer+(dmemin+x)) );
		s16 out( *(s16 *)(Buffer+(dmemout+x)) );

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
