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

#ifndef HLEAUDIO_AUDIOHLE_H_
#define HLEAUDIO_AUDIOHLE_H_

#include "Base/Types.h"

//
//	N.B. This source code is derived from Azimer's Audio plugin (v0.55?)
//	and modified by StrmnNrmn to work with Daedalus PSP. Thanks Azimer!
//	Drop me a line if you get chance :)
//

/* Audio commands: ABI 1 */
/*
#define	A_SPNOOP				0
#define	A_ADPCM					1
#define	A_CLEARBUFF				2
#define	A_ENVMIXER				3
#define	A_LOADBUFF				4
#define	A_RESAMPLE				5
#define A_SAVEBUFF				6
#define A_SEGMENT				7
#define A_SETBUFF				8
#define A_SETVOL				9
#define A_DMEMMOVE              10
#define A_LOADADPCM             11
#define A_MIXER					12
#define A_INTERLEAVE            13
#define A_POLEF                 14
#define A_SETLOOP               15
*/
#define ACMD_SIZE 32
//#define BUF_SIZE                16	//Normal 16bit
//#define ADR_SIZE                24	//Normal 24bit
#define BUF_SIZE                                                               \
  15;                                                                          \
  u32:                                                                         \
  1 // This will force audio buffer access to only 15bit	//Corn
#define ADR_SIZE                                                               \
  23;                                                                          \
  u32:                                                                         \
  1 // This will force RDRAM address to 0 -> 0x7FFFFF (8MB) //Corn
/*
 * Audio flags
 */

#define A_INIT 0x01
#define A_CONTINUE 0x00
#define A_LOOP 0x02
#define A_OUT 0x02
#define A_LEFT 0x02
#define A_RIGHT 0x00
#define A_VOL 0x04
#define A_RATE 0x00
#define A_AUX 0x08
#define A_NOAUX 0x00
#define A_MAIN 0x00
#define A_MIX 0x10

//------------------------------------------------------------------------------------------

struct SAbi1ClearBuffer {
  unsigned Count : BUF_SIZE;
  unsigned : 16; // Unknown/unused
  unsigned Address : BUF_SIZE;
  unsigned : 8; // Unknown/unused
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi1ClearBuffer ) == 8 );

struct SAbi1EnvMixer {
  unsigned Address : ADR_SIZE;
  unsigned : 8;
  unsigned : 16;
  unsigned Flags : 8;
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi1EnvMixer ) == 8 );

struct SAbi1Mixer {
  unsigned DmemOut : BUF_SIZE;
  unsigned DmemIn : BUF_SIZE;
  signed Gain : 16;
  unsigned : 8; // Unknown/unused
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi1Mixer ) == 8 );

struct SAbi1Resample {
  unsigned Address : ADR_SIZE;
  unsigned : 8;
  unsigned Pitch : 16;
  unsigned Flags : 8;
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi1Resample ) == 8 );

struct SAbi1ADPCM {
  unsigned Address : ADR_SIZE;
  unsigned : 8;
  unsigned Gain : 16;
  unsigned Flags : 8;
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi1ADPCM ) == 8 );

struct SAbi2Mixer {
  unsigned DmemOut : BUF_SIZE;
  unsigned DmemIn : BUF_SIZE;
  signed Gain : 16;
  unsigned Count : 8;
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi2Mixer ) == 8 );

struct SAbi2Resample {
  unsigned Address : ADR_SIZE;
  unsigned : 8;
  unsigned Pitch : 16;
  unsigned Flags : 8;
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi2Resample ) == 8 );

struct SAbi1LoadBuffer {
  unsigned Address : ADR_SIZE;
  unsigned : 8;
  unsigned : 24; // Unknown/unused
  unsigned : 8;  // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi1LoadBuffer ) == 8 );

struct SAbi1SaveBuffer {
  unsigned Address : ADR_SIZE;
  unsigned : 8;
  unsigned : 24; // Unknown/unused
  unsigned : 8;  // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi1SaveBuffer ) == 8 );

struct SAbi1SetSegment {
  unsigned Address : ADR_SIZE;
  unsigned Segment : 8;
  unsigned : 24; // Unknown/unused
  unsigned : 8;  // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi1SetSegment ) == 8 );

struct SAbi1SetLoop {
  unsigned LoopVal : ADR_SIZE;
  unsigned : 8;
  unsigned : 24; // Unknown/unused
  unsigned : 8;  // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi1SetLoop ) == 8 );

struct SAbi1SetBuffer {
  unsigned Count : BUF_SIZE;
  unsigned Out : BUF_SIZE;
  unsigned In : BUF_SIZE;
  unsigned Flags : 8;
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi1SetBuffer ) == 8 );

struct SAbi1DmemMove {
  unsigned Count : BUF_SIZE;
  unsigned Dst : BUF_SIZE;
  unsigned Src : BUF_SIZE;
  unsigned : 8;
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi1DmemMove ) == 8 );

struct SAbi1LoadADPCM {
  unsigned Address : ADR_SIZE;
  unsigned : 8;
  unsigned Count : BUF_SIZE;
  unsigned : 8; // Unknown/unused
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi1LoadADPCM ) == 8 );

struct SAbi1Interleave {
  unsigned LAddr : BUF_SIZE;
  unsigned RAddr : BUF_SIZE;
  unsigned : 24; // Unknown/unused
  unsigned : 8;  // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi1Interleave ) == 8 );

struct SAbi2ClearBuffer {
  unsigned Count : BUF_SIZE;
  unsigned : 16; // Unknown/unused
  unsigned Address : BUF_SIZE;
  unsigned : 8; // Unknown/unused
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi2ClearBuffer ) == 8 );

struct SAbi2LoadBuffer {
  unsigned SrcAddr : ADR_SIZE;
  unsigned : 8;
  unsigned DstAddr : 12;
  unsigned Count : 12;
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi2LoadBuffer ) == 8 );

struct SAbi2SaveBuffer {
  unsigned DstAddr : ADR_SIZE;
  unsigned : 8;
  unsigned SrcAddr : 12;
  unsigned Count : 12;
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi2SaveBuffer ) == 8 );

struct SAbi2SetLoop {
  unsigned LoopVal : ADR_SIZE;
  unsigned : 8;
  unsigned : 24; // Unknown/unused
  unsigned : 8;  // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi2SetLoop ) == 8 );

struct SAbi2SetBuffer {
  unsigned Count : BUF_SIZE;
  unsigned Out : BUF_SIZE;
  unsigned In : BUF_SIZE;
  unsigned Flags : 8; // Actually used?
  unsigned : 8;       // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi2SetBuffer ) == 8 );

struct SAbi2DmemMove {
  unsigned Count : BUF_SIZE;
  unsigned Dst : BUF_SIZE;
  unsigned Src : BUF_SIZE;
  unsigned : 8;
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi2DmemMove ) == 8 );

struct SAbi2LoadADPCM {
  unsigned Address : ADR_SIZE;
  unsigned : 8;
  unsigned Count : BUF_SIZE;
  unsigned : 8; // Unknown/unused
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi2LoadADPCM ) == 8 );

struct SAbi2Deinterleave {
  unsigned Out : BUF_SIZE;
  unsigned In : BUF_SIZE;
  unsigned Count : BUF_SIZE;
  unsigned : 8;
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi2Deinterleave ) == 8 );

struct SAbi2Interleave {
  unsigned RAddr : BUF_SIZE;
  unsigned LAddr : BUF_SIZE;
  unsigned OutAddr : 12; // XXXX Not sure if this is correct.
  unsigned Count : 12;   // Might be :16 :8 (with count implicitly *16)?
  unsigned : 8;          // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi2Interleave ) == 8 );

struct SAbi3LoadADPCM {
  unsigned Address : ADR_SIZE;
  unsigned : 8;
  unsigned Count : BUF_SIZE;
  unsigned : 8; // Unknown/unused
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi3LoadADPCM ) == 8 );

struct SAbi3SetLoop {
  unsigned LoopVal : ADR_SIZE;
  unsigned : 8;
  unsigned : 24; // Unknown/unused
  unsigned : 8;  // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi3SetLoop ) == 8 );

struct SAbi3DmemMove {
  unsigned Count : BUF_SIZE;
  unsigned Dst : BUF_SIZE;
  unsigned Src : BUF_SIZE;
  unsigned : 8;
  unsigned : 8; // Command
};
// DAEDALUS_STATIC_ASSERT( sizeof( SAbi3DmemMove ) == 8 );

struct AudioHLECommand {
  union {
    u64 _u64;

    struct {
      u32 cmd1;
      u32 cmd0;
    };

    SAbi1ClearBuffer Abi1ClearBuffer;
    SAbi1EnvMixer Abi1EnvMixer;
    SAbi1Mixer Abi1Mixer;
    SAbi1Resample Abi1Resample;
    SAbi1ADPCM Abi1ADPCM;
    SAbi1LoadBuffer Abi1LoadBuffer;
    SAbi1SaveBuffer Abi1SaveBuffer;
    SAbi1Interleave Abi1Interleave;
    SAbi1SetSegment Abi1SetSegment;
    SAbi1SetLoop Abi1SetLoop;
    SAbi1SetBuffer Abi1SetBuffer;
    SAbi1DmemMove Abi1DmemMove;
    SAbi1LoadADPCM Abi1LoadADPCM;

    SAbi2ClearBuffer Abi2ClearBuffer;
    SAbi2Mixer Abi2Mixer;
    SAbi2Resample Abi2Resample;
    SAbi2LoadBuffer Abi2LoadBuffer;
    SAbi2SaveBuffer Abi2SaveBuffer;
    SAbi2SetLoop Abi2SetLoop;
    SAbi2SetBuffer Abi2SetBuffer;
    SAbi2Deinterleave Abi2Deinterleave;
    SAbi2Interleave Abi2Interleave;
    SAbi2DmemMove Abi2DmemMove;
    SAbi2LoadADPCM Abi2LoadADPCM;

    SAbi3SetLoop Abi3SetLoop;
    SAbi3DmemMove Abi3DmemMove;
    SAbi3LoadADPCM Abi3LoadADPCM;

    struct {
      int : 32;
      int : 24;
      u32 cmd : 5;
      u32 top : 3;
    };
  };
};
// DAEDALUS_STATIC_ASSERT( sizeof( AudioHLECommand ) == 8 );

using AudioHLEInstruction = void (*)(AudioHLECommand command);

// ABI_BUFFER
void CLEARBUFF(AudioHLECommand command);
void CLEARBUFF2(AudioHLECommand command);
void CLEARBUFF3(AudioHLECommand command);
void DMEMMOVE(AudioHLECommand command);
void DMEMMOVE2(AudioHLECommand command);
void DMEMMOVE3(AudioHLECommand command);
void DUPLICATE2(AudioHLECommand command);
void LOADBUFF(AudioHLECommand command);
void LOADBUFF2(AudioHLECommand command);
void LOADBUFF3(AudioHLECommand command);
void SAVEBUFF(AudioHLECommand command);
void SAVEBUFF2(AudioHLECommand command);
void SAVEBUFF3(AudioHLECommand command);
void SEGMENT(AudioHLECommand command);
void SEGMENT2(AudioHLECommand command);
void SETBUFF(AudioHLECommand command);
void SETBUFF2(AudioHLECommand command);
void SETLOOP(AudioHLECommand command);
void SETLOOP2(AudioHLECommand command);
void SETLOOP3(AudioHLECommand command);

void ADDMIXER(AudioHLECommand command);
void ADPCM(AudioHLECommand command);
void ADPCM2(AudioHLECommand command);
void ADPCM3(AudioHLECommand command);
void ENVMIXER(AudioHLECommand command);
void ENVMIXER2(AudioHLECommand command);
void ENVMIXER3(AudioHLECommand command);
void ENVMIXER_GE(AudioHLECommand command);
void ENVSETUP1(AudioHLECommand command);
void ENVSETUP2(AudioHLECommand command);
void FILTER2(AudioHLECommand command);
void HILOGAIN(AudioHLECommand command);
void DEINTERLEAVE2(AudioHLECommand command);
void INTERLEAVE(AudioHLECommand command);
void INTERLEAVE2(AudioHLECommand command);
void INTERLEAVE3(AudioHLECommand command);
void LOADADPCM(AudioHLECommand command);
void LOADADPCM2(AudioHLECommand command);
void LOADADPCM3(AudioHLECommand command);
void MIXER(AudioHLECommand command);
void MIXER2(AudioHLECommand command);
void MIXER3(AudioHLECommand command);
void MP3(AudioHLECommand command);
//	void MP3ADDY(AudioHLECommand command );
void POLEF(AudioHLECommand command );
void RESAMPLE(AudioHLECommand command);
void RESAMPLE2(AudioHLECommand command);
void RESAMPLE3(AudioHLECommand command);
void SETVOL(AudioHLECommand command);
void SETVOL3(AudioHLECommand command);
void SPNOOP(AudioHLECommand command);
void UNKNOWN(AudioHLECommand command);

// These must be defined...
#include "Core/Memory.h"

// MMmm, why not use the defines from Memory.h?
// ToDo : remove these and use the ones already provided by the core?
#define dmem ((u8 *)g_pMemoryBuffers[MEM_SP_MEM] + SP_DMA_DMEM)
#define imem ((u8 *)g_pMemoryBuffers[MEM_SP_MEM] + SP_DMA_IMEM)
#define rdram ((u8 *)g_pMemoryBuffers[MEM_RD_RAM])

// Use these functions to interface with the HLE Audio...
void Audio_Ucode();
void Audio_Reset();

#endif // HLEAUDIO_AUDIOHLE_H_
