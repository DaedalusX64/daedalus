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
//

#include "stdafx.h"
#include "audiohle.h"
#include "AudioHLEProcessor.h"

#include "OSHLE/ultra_sptask.h"

// Audio UCode lists
// Dummy UCode Handler
//
static void SPU( AudioHLECommand command ){}
//
//     ABI ? : Unknown or unsupported UCode
//
AudioHLEInstruction ABIUnknown [0x20] = { // Unknown ABI
	SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU,
	SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU,
	SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU,
	SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU
};
//---------------------------------------------------------------------------------------------
//
//     ABI 1 : Mario64, WaveRace USA, Golden Eye 007, Quest64, SF Rush
//				 60% of all games use this.  Distributed 3rd Party ABI
//
extern AudioHLEInstruction ABI1[0x20];
//---------------------------------------------------------------------------------------------
//
//     ABI 2 : WaveRace JAP, MarioKart 64, Mario64 JAP RumbleEdition, 
//				 Yoshi Story, Pokemon Games, Zelda64, Zelda MoM (miyamoto) 
//				 Most NCL or NOA games (Most commands)
extern AudioHLEInstruction ABI2[0x20];
//---------------------------------------------------------------------------------------------
//
//     ABI 3 : DK64, Perfect Dark, Banjo Kazooi, Banjo Tooie
//				 All RARE games except Golden Eye 007
//
extern AudioHLEInstruction ABI3[0x20];
//---------------------------------------------------------------------------------------------
//
//     ABI 5 : Factor 5 - MoSys/MusyX
//				 Rogue Squadron, Tarzan, Hydro Thunder, and TWINE
//				 Indiana Jones and Battle for Naboo (?)
//---------------------------------------------------------------------------------------------
//
// Below functions were updated
//

//---------------------------------------------------------------------------------------------
AudioHLEInstruction *ABI=ABIUnknown;
//---------------------------------------------------------------------------------------------

//*****************************************************************************
//
//*****************************************************************************
inline void Audio_Ucode_Detect(OSTask * pTask)
{
	if (*(u32*)(g_pu8RamBase + (u32)pTask->t.ucode_data + 0) != 0x01)
	{
		if (*(u32*)(g_pu8RamBase + (u32)pTask->t.ucode_data + 0) == 0x0F)
			ABI=ABIUnknown;
		else
			ABI=ABI3;
	}
	else
	{
		if (*(u32*)(g_pu8RamBase + (u32)pTask->t.ucode_data + 0x30) == 0xF0000F00)
			ABI=ABI1;
		else
			ABI=ABI2;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void Audio_Ucode()
{
	DAEDALUS_PROFILE( "HLEMain::Audio_Ucode" );

	OSTask * pTask = (OSTask *)(g_pu8SpMemBase + 0x0FC0);
	static u32	last_code_base = 0;
	u32 code_base = (u32)pTask->t.ucode & 0x1fffffff;

	// Only detect ABI once, unless is a different ucode
	//
	if ( last_code_base != code_base )
	{
		last_code_base = code_base;
		Audio_Ucode_Detect( pTask );
	}
	
	//gAudioHLEState.LoopVal = 0;
	//memset( gAudioHLEState.Segments, 0, sizeof( gAudioHLEState.Segments ) );
	u32 * p_alist = (u32 *)(g_pu8RamBase + (u32)pTask->t.data_ptr);
	u32 ucode_size = (pTask->t.data_size / 8) + 1;

	AudioHLECommand command;

	while(ucode_size--)
    {
        command.cmd0 = *p_alist++;
        command.cmd1 = *p_alist++;
        ABI[command.cmd](command);
		//printf("%08X %08X\n",command.cmd0,command.cmd1);
	}
}

