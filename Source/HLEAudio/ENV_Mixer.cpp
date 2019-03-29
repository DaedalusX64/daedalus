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
short hleMixerWorkArea[256];



static u32 gEnv_t3 {}, gEnv_s5 {}, gEnv_s6 {};
static u16 env[8] {};

inline s32		FixedPointMulFull16( s32 a, s32 b )
{
	return s32( ( (s64)a * (s64)b ) >> 16 );
}

s16 LoadMixer16(int offset)
{
	return *(s16 *)(hleMixerWorkArea + offset);
}

s32 LoadMixer32(int offset)
{
	return *(s32 *)(hleMixerWorkArea + offset);
}

void SaveMixer16(int offset, s16 value)
{
	*(s16 *)(hleMixerWorkArea + offset) = value;
}

void SaveMixer32(int offset, s32 value)
{
	*(s32 *)(hleMixerWorkArea + offset) = value;
}


static inline s32 mixer_macc(s32* Acc, s32* AdderStart, s32* AdderEnd, s32 Ramp)
{
	s32 volume;

#if 0
	s64 product, product_shifted;
	/*** TODO!  It looks like my C translation of Azimer's assembly code ... ***/
	product = (s64)(*AdderEnd) * (s64)Ramp;
	product_shifted = product >> 16;

	volume = (*AdderEnd - *AdderStart) / 8;
	*Acc = *AdderStart;
	*AdderStart = *AdderEnd;
	*AdderEnd = (s32)(product_shifted & 0xFFFFFFFFul);
#else
	/*** ... comes out to something not the same as the C code he commented. ***/
	volume = (*AdderEnd - *AdderStart) >> 3;
	*Acc = *AdderStart;
	*AdderEnd   = (s32)((((s64)(*AdderEnd) * (s64)Ramp) >> 16) & 0xFFFFFFFF);
	*AdderStart = (s32)((((s64)(*Acc)      * (s64)Ramp) >> 16) & 0xFFFFFFFF);
#endif
	return (volume);
}


void ENVSETUP1( AudioHLECommand command )
{
	//fprintf (dfile, "ENVSETUP1: cmd0 = %08X, cmd1 = %08X\n", command.cmd0, command.cmd1);
	gEnv_t3 = command.cmd0 & 0xFFFF;
	u16 tmp	{(command.cmd0 >> 0x8) & 0xFF00};
	env[4] = (u16)tmp;
	tmp +=  gEnv_t3;
	env[5] = (u16)tmp;
	gEnv_s5 = command.cmd1 >> 0x10;
	gEnv_s6 = command.cmd1 & 0xFFFF;
	//fprintf (dfile, "	gEnv_t3 = %X / gEnv_s5 = %X / gEnv_s6 = %X / env[4] = %X / env[5] = %X\n", gEnv_t3, gEnv_s5, gEnv_s6, env[4], env[5]);
}

void ENVSETUP2( AudioHLECommand command )
{
	//fprintf (dfile, "ENVSETUP2: cmd0 = %08X, cmd1 = %08X\n", command.cmd0, command.cmd1);
	u16 tmp = (command.cmd1 >> 0x10);
	env[0] = (u16)tmp;
	tmp += gEnv_s5;
	env[1] = (u16)tmp;
	tmp = (command.cmd1 & 0xfff);
	env[2] = (u16)tmp;
	tmp += gEnv_s6;
	env[3] = (u16)tmp;

	//fprintf (dfile, "	env[0] = %X / env[1] = %X / env[2] = %X / env[3] = %X\n", env[0], env[1], env[2], env[3]);
}

void ENVSETUP3( AudioHLECommand command)
{

}

void ENVMIXER( AudioHLECommand command )
{
	u8	flags( command.Abi1EnvMixer.Flags );
	u32 address( command.Abi1EnvMixer.Address );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	// Restore this for now..
	//static
	// ********* Make sure these conditions are met... ***********
	/*if ((InBuffer | OutBuffer | AuxA | AuxC | AuxE | Count) & 0x3) {
	MessageBox (NULL, "Unaligned EnvMixer... please report this to Azimer with the following information: RomTitle, Place in the rom it occurred, and any save state just before the error", "AudioHLE Error", MB_OK);
	}*/
	// ------------------------------------------------------------
	s16 *inp {(s16 *)(gAudioHLEState.Buffer+gAudioHLEState.InBuffer)};
	s16 *out {(s16 *)(gAudioHLEState.Buffer+gAudioHLEState.OutBuffer)};
	s16 *aux1 {(s16 *)(gAudioHLEState.Buffer+gAudioHLEState.AuxA)};
	s16 *aux2 {(s16 *)(gAudioHLEState.Buffer+gAudioHLEState.AuxC)};
	s16 *aux3 {(s16 *)(gAudioHLEState.Buffer+gAudioHLEState.AuxE)};
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
		LVol = ((gAudioHLEState.VolLeft  * gAudioHLEState.VolRampLeft));
		RVol = ((gAudioHLEState.VolRight * gAudioHLEState.VolRampRight));
		Wet = gAudioHLEState.EnvWet;
		Dry = gAudioHLEState.EnvDry; // Save Wet/Dry values
		LTrg = (gAudioHLEState.VolTrgLeft << 16);
		RTrg = (gAudioHLEState.VolTrgRight << 16); // Save Current Left/Right Targets
		LAdderStart = gAudioHLEState.VolLeft  << 16;
		RAdderStart = gAudioHLEState.VolRight << 16;
		LAdderEnd = gAudioHLEState.VolLeft;
		RAdderEnd = gAudioHLEState.VolRight;
		RRamp = gAudioHLEState.VolRampRight;
		LRamp = gAudioHLEState.VolRampLeft;
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

	for (s32 y {}; y < gAudioHLEState.Count; y += 0x10)
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

void ENVMIXER2( AudioHLECommand command )
{
	//fprintf (dfile, "ENVMIXER: cmd0 = %08X, cmd1 = %08X\n", command.cmd0, command.cmd1);
	s16 vec9 {}, vec10 {};

	s16 *buffs3 {(s16 *)(gAudioHLEState.Buffer + ((command.cmd0 >> 0x0c)&0x0ff0))};
	s16 *bufft6 {(s16 *)(gAudioHLEState.Buffer + ((command.cmd1 >> 0x14)&0x0ff0))};
	s16 *bufft7 {(s16 *)(gAudioHLEState.Buffer + ((command.cmd1 >> 0x0c)&0x0ff0))};
	s16 *buffs0 {(s16 *)(gAudioHLEState.Buffer + ((command.cmd1 >> 0x04)&0x0ff0))};
	s16 *buffs1 {(s16 *)(gAudioHLEState.Buffer + ((command.cmd1 << 0x04)&0x0ff0))};


	s16 v2[8] {};
	v2[0] = 0 - (s16)((command.cmd0 & 0x2) >> 1);
	v2[1] = 0 - (s16)((command.cmd0 & 0x1));
	v2[2] = 0 - (s16)((command.cmd0 & 0x8) >> 1);
	v2[3] = 0 - (s16)((command.cmd0 & 0x4) >> 1);

	s32 count {(s32)(command.cmd0 >> 8) & 0xff};

	u32 adder {};

	if (!isMKABI)
	{
		gEnv_s5 *= 2; gEnv_s6 *= 2; gEnv_t3 *= 2;
		adder = 0x10;
	}
	else
	{
		command.cmd0 = 0;
		adder = 0x8;
		gEnv_t3 = 0;
	}


	while (count > 0)
	{
		int temp {};
		for (int x=0; x < 0x8; x++)
		{
			vec9  = (s16)(((s32)buffs3[x^1] * (u32)env[0]) >> 0x10) ^ v2[0];
			vec10 = (s16)(((s32)buffs3[x^1] * (u32)env[2]) >> 0x10) ^ v2[1];
			temp = bufft6[x^1] + vec9;
			bufft6[x^1] = Saturate<s16>( temp );
			temp = bufft7[x^1] + vec10;
			bufft7[x^1] = Saturate<s16>( temp );
			vec9  = (s16)(((s32)vec9  * (u32)env[4]) >> 0x10) ^ v2[2];
			vec10 = (s16)(((s32)vec10 * (u32)env[4]) >> 0x10) ^ v2[3];
			if (command.cmd0 & 0x10)
			{
				temp = buffs0[x^1] + vec10;
				buffs0[x^1] = Saturate<s16>( temp );
				temp = buffs1[x^1] + vec9;
				buffs1[x^1] = Saturate<s16>( temp );
			}
			else
			{
				temp = buffs0[x^1] + vec9;
				buffs0[x^1] = Saturate<s16>( temp );
				temp = buffs1[x^1] + vec10;
				buffs1[x^1] = Saturate<s16>( temp );
			}
		}

		if (!isMKABI)
		for (int x = 0x8; x < 0x10; x++)
		{
			vec9  = (s16)(((s32)buffs3[x^1] * (u32)env[1]) >> 0x10) ^ v2[0];
			vec10 = (s16)(((s32)buffs3[x^1] * (u32)env[3]) >> 0x10) ^ v2[1];
			temp = bufft6[x^1] + vec9;
			bufft6[x^1] = Saturate<s16>( temp );
			temp = bufft7[x^1] + vec10;
			bufft7[x^1] = Saturate<s16>( temp );
			vec9  = (s16)(((s32)vec9  * (u32)env[5]) >> 0x10) ^ v2[2];
			vec10 = (s16)(((s32)vec10 * (u32)env[5]) >> 0x10) ^ v2[3];
			if (command.cmd0 & 0x10)
			{
				temp = buffs0[x^1] + vec10;
				buffs0[x^1] = Saturate<s16>( temp );
				temp = buffs1[x^1] + vec9;
				buffs1[x^1] = Saturate<s16>( temp );
			}
			else
			{
				temp = buffs0[x^1] + vec9;
				buffs0[x^1] = Saturate<s16>( temp );
				temp = buffs1[x^1] + vec10;
				buffs1[x^1] = Saturate<s16>( temp );
			}
		}
		bufft6 += adder; bufft7 += adder;
		buffs0 += adder; buffs1 += adder;
		buffs3 += adder; count  -= adder;
		env[0] += gEnv_s5; env[1] += gEnv_s5;
		env[2] += gEnv_s6; env[3] += gEnv_s6;
		env[4] += gEnv_t3; env[5] += gEnv_t3;
	}
}

void ENVMIXER3( AudioHLECommand command )
{
	u8 flags {(u8)((command.cmd0 >> 16) & 0xff)};
	u32 addy {(command.cmd1 & 0xFFFFFF)};

 	s16 *inp {(s16 *)(gAudioHLEState.Buffer+0x4F0)};
	s16 *out {(s16 *)(gAudioHLEState.Buffer+0x9D0)};
	s16 *aux1 {(s16 *)(gAudioHLEState.Buffer+0xB40)};
	s16 *aux2 {(s16 *)(gAudioHLEState.Buffer+0xCB0)};
	s16 *aux3 {(s16 *)(gAudioHLEState.Buffer+0xE20)};

	s32 MainR {},MainL {},AuxR {}, AuxL {};
	s32 i1 {}, o1 {}, a1 {}, a2 {}, a3 {};
	s32 LAdder {}, LAcc {}, LVol {}, RAdder {}, RAcc {}, RVol {};
	s16 RSig {}, LSig {}, Wet {}, Dry {}, LTrg {}, RTrg {};

	gAudioHLEState.VolRight = (s16)command.cmd0;

	s16* buff {(s16*)(rdram+addy)};

	if (flags & A_INIT)
	{
		LAdder = gAudioHLEState.VolRampLeft / 8;
		LAcc  = 0;
		LVol  = gAudioHLEState.VolLeft;
		LSig = (s16)(gAudioHLEState.VolRampLeft >> 16);

		RAdder = gAudioHLEState.VolRampRight / 8;
		RAcc  = 0;
		RVol  = gAudioHLEState.VolRight;
		RSig = (s16)(gAudioHLEState.VolRampRight >> 16);

		Wet = gAudioHLEState.EnvWet;
		Dry = gAudioHLEState.EnvDry; // Save Wet/Dry values
		LTrg = gAudioHLEState.VolTrgLeft; RTrg = gAudioHLEState.VolTrgRight; // Save Current Left/Right Targets
	}
	else
	{
		Wet    = *(s16 *)(buff +  0); // 0-1
		Dry    = *(s16 *)(buff +  2); // 2-3
		LTrg   = *(s16 *)(buff +  4); // 4-5
		RTrg   = *(s16 *)(buff +  6); // 6-7
		LAdder = *(s32 *)(buff +  8); // 8-9 (buff is a 16bit pointer)
		RAdder = *(s32 *)(buff + 10); // 10-11
		LAcc   = *(s32 *)(buff + 12); // 12-13
		RAcc   = *(s32 *)(buff + 14); // 14-15
		LVol   = *(s32 *)(buff + 16); // 16-17
		RVol   = *(s32 *)(buff + 18); // 18-19
		LSig   = *(s16 *)(buff + 20); // 20-21
		RSig   = *(s16 *)(buff + 22); // 22-23
		//u32 test  = *(s32 *)(buff + 24); // 22-23
		//if (test != 0x13371337)
		//	__asm int 3;
	}


	//if(!(flags&A_AUX)) {
	//	AuxIncRate=0;
	//	aux2=aux3=zero;
	//}

	for (s32 y {}; y < (0x170/2); y++) {

		// Left
		LAcc += LAdder;
		LVol += (LAcc >> 16);
		LAcc &= 0xFFFF;

		// Right
		RAcc += RAdder;
		RVol += (RAcc >> 16);
		RAcc &= 0xFFFF;
// ****************************************************************
		// Clamp Left
		if (LSig >= 0) { // VLT
			if (LVol > LTrg) {
				LVol = LTrg;
			}
		} else { // VGE
			if (LVol < LTrg) {
				LVol = LTrg;
			}
		}

		// Clamp Right
		if (RSig >= 0) { // VLT
			if (RVol > RTrg) {
				RVol = RTrg;
			}
		} else { // VGE
			if (RVol < RTrg) {
				RVol = RTrg;
			}
		}
// ****************************************************************
		MainL = ((Dry * LVol) + 0x4000) >> 15;
		MainR = ((Dry * RVol) + 0x4000) >> 15;

		o1 = out [y^1];
		a1 = aux1[y^1];
		i1 = inp [y^1];

		o1+=((i1*MainL)+0x4000)>>15;
		a1+=((i1*MainR)+0x4000)>>15;

// ****************************************************************

		o1 = Saturate<s16>( o1 );
		a1 = Saturate<s16>( a1 );

// ****************************************************************

		out[y^1]=o1;
		aux1[y^1]=a1;

// ****************************************************************
		//if (!(flags&A_AUX)) {
			a2 = aux2[y^1];
			a3 = aux3[y^1];

			AuxL  = ((Wet * LVol) + 0x4000) >> 15;
			AuxR  = ((Wet * RVol) + 0x4000) >> 15;

			a2+=((i1*AuxL)+0x4000)>>15;
			a3+=((i1*AuxR)+0x4000)>>15;

			a2 = Saturate<s16>( a2 );
			a3 = Saturate<s16>( a3 );

			aux2[y^1]=a2;
			aux3[y^1]=a3;
		}
	//}

	*(s16 *)(buff +  0) = Wet; // 0-1
	*(s16 *)(buff +  2) = Dry; // 2-3
	*(s16 *)(buff +  4) = LTrg; // 4-5
	*(s16 *)(buff +  6) = RTrg; // 6-7
	*(s32 *)(buff +  8) = LAdder; // 8-9 (buff is a 16bit pointer)
	*(s32 *)(buff + 10) = RAdder; // 10-11
	*(s32 *)(buff + 12) = LAcc; // 12-13
	*(s32 *)(buff + 14) = RAcc; // 14-15
	*(s32 *)(buff + 16) = LVol; // 16-17
	*(s32 *)(buff + 18) = RVol; // 18-19
	*(s16 *)(buff + 20) = LSig; // 20-21
	*(s16 *)(buff + 22) = RSig; // 22-23
	//*(u32 *)(buff + 24) = 0x13371337; // 22-23
}

void SETVOL( AudioHLECommand command )
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

void SETVOL3( AudioHLECommand command )
{
	u8 Flags {(u8)(command.cmd0 >> 0x10)};
	if (Flags & 0x4)
	{ // 288
		if (Flags & 0x2)
		{ // 290
			gAudioHLEState.VolLeft  = (s16)command.cmd0; // 0x50
			gAudioHLEState.EnvDry	= (s16)(command.cmd1 >> 16); // 0x4E
			gAudioHLEState.EnvWet	= (s16)command.cmd1; // 0x4C
		}
		else
		{
			gAudioHLEState.VolTrgRight  = (s16)command.cmd0; // 0x46
			//gAudioHLEState.VolRampRight = (u16)(command.cmd1 >> 16) | (s32)(s16)(command.cmd1 << 0x10);
			gAudioHLEState.VolRampRight = command.cmd1; // 0x48/0x4A
		}
	}
	else
	{
		gAudioHLEState.VolTrgLeft  = (s16)command.cmd0; // 0x40
		gAudioHLEState.VolRampLeft = command.cmd1; // 0x42/0x44
	}
}
