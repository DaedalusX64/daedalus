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



struct AudioHLEState
{
	void	ClearBuffer( u16 addr, u16 count );

	void	EnvMixer( u8 flags, u32 address );

	void	Resample( u8 flags, u32 pitch, u32 address );

	void	ADPCMDecode( u8 flags, u32 address );

	void	LoadBuffer( u32 address );
	void	SaveBuffer( u32 address );
	void	LoadBuffer( u16 dram_dst, u32 ram_src, u16 count );
	void	SaveBuffer( u32 ram_dst, u16 dmem_src, u16 count );

	void	SetSegment( u8 segment, u32 address );
	void	SetLoop( u32 loopval );
	void	SetBuffer( u8 flags, u16 in, u16 out, u16 count );

	void	DmemMove( u16 dst, u16 src, u16 count );
	void	LoadADPCM( u32 address, u16 count );

	void	Interleave( u16 laddr, u16 raddr );								// Uses OutBuffer/Count
	void	Interleave( u16 outaddr, u16 laddr, u16 raddr, u16 count );
	void	Deinterleave( u16 outaddr, u16 inaddr, u16 count );

	void	Mixer( u16 dmemout, u16 dmemin, s32 gain, u16 count );
	void	Mixer( u16 dmemout, u16 dmemin, s32 gain );

private:
	void	ExtractSamplesScale( s32 * output, u32 inPtr, s32 vscale ) const;
	void	ExtractSamples( s32 * output, u32 inPtr ) const;


public:
	u8		Buffer[0x10000]; // Seems excesively large? 0x1000 should be enough.
	u16		ADPCMTable[0x88];
	s16		MixerWorkArea[256];
	
	u32		Segments[16];		// 0x0320
	// T8 = 0x360
	u16		InBuffer;			// 0x0000(T8)
	u16		OutBuffer;			// 0x0002(T8)
	u16		Count;				// 0x0004(T8)

	s16		VolLeft;			// 0x0006(T8)
	s16		VolRight;			// 0x0008(T8)

	u16		AuxA;				// 0x000A(T8)
	u16		AuxC;				// 0x000C(T8)
	u16		AuxE;				// 0x000E(T8)

	u32		LoopVal;			// 0x0010(T8) // Value set by A_SETLOOP : Possible conflict with SETVOLUME???

	s16		VolTrgLeft;			// 0x0010(T8)
	s32		VolRampLeft;		//

	s16		VolTrgRight;		//
	s32		VolRampRight;		//

	s16		EnvDry;				// 0x001C(T8)
	s16		EnvWet;				// 0x001E(T8)

};

extern AudioHLEState gAudioHLEState;
