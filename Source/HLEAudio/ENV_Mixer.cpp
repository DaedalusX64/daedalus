#include "stdafx.h"

#include <string.h>

#include "audiohle.h"
#include "AudioHLEProcessor.h"

#include "Core/Memory.h"
#include "Math/MathUtil.h"

#include "Debug/DBGConsole.h"

extern bool isMKABI;
extern bool isZeldaABI;

static u32 gEnv_t3 {}, gEnv_s5 {}, gEnv_s6 {};
static u16 env[8] {};

inline u16 Sample_Mask( u32 x )
{
	return (u16)( x & 0xffff );
}


void ENVSETUP1(AudioHLECommand command)
{
  //fprintf (dfile, "ENVSETUP1: cmd0 = %08X, cmd1 = %08X\n", command.cmd0, command.cmd1);
  	gEnv_t3 = command.cmd0 & 0xFFFF;
  	u32 tmp	= (command.cmd0 >> 0x8) & 0xFF00;
  	env[4] = Sample_Mask(tmp);
  	env[5] = Sample_Mask(tmp + gEnv_t3);
  	gEnv_s5 = command.cmd1 >> 0x10;
  	gEnv_s6 = command.cmd1 & 0xFFFF;
  	//fprintf (dfile, "	gEnv_t3 = %X / gEnv_s5 = %X / gEnv_s6 = %X / env[4] = %X / env[5] = %X\n", gEnv_t3, gEnv_s5, gEnv_s6, env[4], env[5]);
}

void ENVSETUP2(AudioHLECommand command)
{
  //fprintf (dfile, "ENVSETUP2: cmd0 = %08X, cmd1 = %08X\n", command.cmd0, command.cmd1);
	u32 tmp1 = (command.cmd1 >> 0x10);
	env[0] = Sample_Mask(tmp1);
	env[1] = Sample_Mask(tmp1 + gEnv_s5);

	u32 tmp2 = command.cmd1 & 0xffff;
	env[2] = Sample_Mask(tmp2);
	env[3] = Sample_Mask(tmp2 + gEnv_s6);
	//fprintf (dfile, "	env[0] = %X / env[1] = %X / env[2] = %X / env[3] = %X\n", env[0], env[1], env[2], env[3]);
}

void ENVSETUP3(AudioHLECommand command)
{

}


void ENVMIXER(AudioHLECommand command)
{
	//static int envmixcnt = 0;
	u8	flags( command.Abi1EnvMixer.Flags );
	u32 address( command.Abi1EnvMixer.Address );// + gAudioHLEState.Segments[(command.cmd1>>24)&0xf];

	gAudioHLEState.EnvMixer( flags, address );
}

void ENVMIXER_GE(AudioHLECommand command)
{
  // Not implemented
}


void ENVMIXER2(AudioHLECommand command)
{
  //fprintf (dfile, "ENVMIXER: cmd0 = %08X, cmd1 = %08X\n", command.cmd0, command.cmd1);
  	s16 vec9, vec10;

  	s16 *buffs3 = (s16 *)(gAudioHLEState.Buffer + ((command.cmd0 >> 0x0c)&0x0ff0));
  	s16 *bufft6 = (s16 *)(gAudioHLEState.Buffer + ((command.cmd1 >> 0x14)&0x0ff0));
  	s16 *bufft7 = (s16 *)(gAudioHLEState.Buffer + ((command.cmd1 >> 0x0c)&0x0ff0));
  	s16 *buffs0 = (s16 *)(gAudioHLEState.Buffer + ((command.cmd1 >> 0x04)&0x0ff0));
  	s16 *buffs1 = (s16 *)(gAudioHLEState.Buffer + ((command.cmd1 << 0x04)&0x0ff0));


  	s16 v2[8];
  	v2[0] = 0 - (s16)((command.cmd0 & 0x2) >> 1);
  	v2[1] = 0 - (s16)((command.cmd0 & 0x1));
  	v2[2] = 0 - (s16)((command.cmd0 & 0x8) >> 1);
  	v2[3] = 0 - (s16)((command.cmd0 & 0x4) >> 1);

  	s32 count = (command.cmd0 >> 8) & 0xff;

  	u32 adder;
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
  		int temp;
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
  		for (int x=0x8; x < 0x10; x++)
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
  		env[0] = Sample_Mask(env[0] + gEnv_s5);
  		env[1] = Sample_Mask(env[1] + gEnv_s5);
  		env[2] = Sample_Mask(env[2] + gEnv_s6);
  		env[3] = Sample_Mask(env[3] + gEnv_s6);
  		env[4] = Sample_Mask(env[4] + gEnv_t3);
  		env[5] = Sample_Mask(env[5] + gEnv_t3);
  	}
}

void ENVMIXER3(AudioHLECommand command)
{
  u8 flags = (u8)((command.cmd0 >> 16) & 0xff);
  	u32 addy = (command.cmd1 & 0xFFFFFF);

   	s16 *inp=(s16 *)(gAudioHLEState.Buffer+0x4F0);
  	s16 *out=(s16 *)(gAudioHLEState.Buffer+0x9D0);
  	s16 *aux1=(s16 *)(gAudioHLEState.Buffer+0xB40);
  	s16 *aux2=(s16 *)(gAudioHLEState.Buffer+0xCB0);
  	s16 *aux3=(s16 *)(gAudioHLEState.Buffer+0xE20);
  	s32 MainR;
  	s32 MainL;
  	s32 AuxR;
  	s32 AuxL;
  	s32 i1,o1,a1,a2,a3;

  	s32 LAdder, LAcc, LVol;
  	s32 RAdder, RAcc, RVol;
  	s16 RSig, LSig; // Most significant part of the Ramp Value
  	s16 Wet, Dry;
  	s16 LTrg, RTrg;

  	gAudioHLEState.VolRight = (s16)command.cmd0;

  	s16* buff = (s16*)(rdram+addy);

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

  	for (s32 y = 0; y < (0x170/2); y++) {

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

void SETVOL(AudioHLECommand command)
{
  // Might be better to unpack these depending on the flags...
  	u8 flags = (u8)((command.cmd0 >> 16) & 0xff);
  	s16 vol = (s16)(command.cmd0 & 0xffff);
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

void SETVOL3(AudioHLECommand command)
{
  u8 Flags = (u8)(command.cmd0 >> 0x10);
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
