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

#include "Base/Types.h"
#include <cstring>

#include "HLEAudio/HLEAudioInternal.h"
#include "HLEAudio/HLEAudioState.h"
#include "Ultra/ultra_sptask.h"
#include "Utility/Profiler.h"

// Audio UCode lists
// Dummy UCode Handler
//
static void SPU(AudioHLECommand) {}
//
//     ABI ? : Unknown or unsupported UCode
//
// std::array<AudioHLEInstruction, 0x20> ABIUnknown = {
AudioHLEInstruction ABIUnknown[0x20] = { // Unknown ABI
    SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU,
    SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU,
    SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU, SPU};
//---------------------------------------------------------------------------------------------
//
//     ABI 1 : Mario64, WaveRace USA, Golden Eye 007, Quest64, SF Rush
//				 60% of all games use this.  Distributed 3rd Party
//ABI
//
extern AudioHLEInstruction ABI1[0x20];
// extern std::array<AudioHLEInstruction, 0x20> ABI1;
//---------------------------------------------------------------------------------------------
//
//     ABI 2 : WaveRace JAP, MarioKart 64, Mario64 JAP RumbleEdition,
//				 Yoshi Story, Pokemon Games, Zelda64, Zelda MoM
//(miyamoto) 				 Most NCL or NOA games (Most commands)
extern AudioHLEInstruction ABI2[0x20];

// extern std::array<AudioHLEInstruction, 0x20> ABI2;
//---------------------------------------------------------------------------------------------
//
//     ABI 3 : DK64, Perfect Dark, Banjo Kazooie, Banjo Tooie
//				 All RARE games except Golden Eye 007
//
extern AudioHLEInstruction ABI3[0x20];
// extern std::array<AudioHLEInstruction, 0x20> ABI3;
//---------------------------------------------------------------------------------------------
//
//     ABI 5 : Factor 5 - MoSys/MusyX
//				 Rogue Squadron, Tarzan, Hydro Thunder, and
//TWINE 				 Indiana Jones and Battle for Naboo (?)
//---------------------------------------------------------------------------------------------
//
// Below functions were updated
//

AudioHLEInstruction *ABI = ABIUnknown;
bool bAudioChanged = false;
extern bool isMKABI;
extern bool isZeldaABI;

//*****************************************************************************
//
//*****************************************************************************
void Audio_Reset() {
  bAudioChanged = false;
  isMKABI = false;
  isZeldaABI = false;
}

//*****************************************************************************
//
//*****************************************************************************
inline void Audio_Ucode_Detect(OSTask *pTask) {
  u8 *p_base = g_pu8RamBase + (uintptr_t)pTask->t.ucode_data;
  if (*(u32 *)(p_base + 0) != 0x01) {
    if (*(u32 *)(p_base + 0x10) == 0x00000001)
      ABI = ABIUnknown;
    else
      ABI = ABI3;
  } else {
    if (*(u32 *)(p_base + 0x30) == 0xF0000F00)
      ABI = ABI1;
    else
      ABI = ABI2;
  }
}

//*****************************************************************************
//
//*****************************************************************************
void Audio_Ucode() {
#ifdef DAEDALUS_PROFILE
  DAEDALUS_PROFILE("HLEMain::Audio_Ucode");
#endif
  OSTask *pTask = (OSTask *)(g_pu8SpMemBase + 0x0FC0);

  // Only detect ABI once per game
  if (!bAudioChanged) {
    bAudioChanged = true;
    Audio_Ucode_Detect(pTask);
  }

  gAudioHLEState.LoopVal = 0;
  memset( gAudioHLEState.Segments, 0, sizeof( gAudioHLEState.Segments ) );

  u32 *p_alist = (u32 *)(g_pu8RamBase + (uintptr_t)pTask->t.data_ptr);
  u32 ucode_size = (pTask->t.data_size >> 3); // ABI5 can return 0 here!!!

  while (ucode_size) {
    AudioHLECommand command;
    command.cmd0 = *p_alist++;
    command.cmd1 = *p_alist++;

    ABI[command.cmd](command);

    --ucode_size;

    // printf("%08X %08X\n",command.cmd0,command.cmd1);
  }
}
