/*
   Copyright (C) 2010 StrmnNrmn

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
#ifndef UCODE_H
#define UCODE_H

#include "../stdafx.h"
#include "UcodeDefs.h"

#define MAX_UCODE		12	// Increase this everytime a new ucode table is added !
#define UCODE_CACHED	MAX_UCODE + 1

typedef void(*MicroCodeInstruction)(MicroCodeCommand command);
#define UcodeFunc(name)	void name(MicroCodeCommand)

extern const MicroCodeInstruction gInstructionLookup[MAX_UCODE][256];

#if defined(DAEDALUS_DEBUG_DISPLAYLIST) || defined(DAEDALUS_ENABLE_PROFILING)
extern const char * gInstructionName[3][256];
#endif

//*****************************************************************************
// GBI1
//*****************************************************************************

UcodeFunc( DLParser_Nothing);
UcodeFunc( DLParser_GBI1_SpNoop );
UcodeFunc( DLParser_GBI1_MoveMem );
UcodeFunc( DLParser_GBI1_Reserved );
UcodeFunc( DLParser_GBI1_RDPHalf_Cont );
UcodeFunc( DLParser_GBI1_RDPHalf_2 );
UcodeFunc( DLParser_GBI1_RDPHalf_1 );
UcodeFunc( DLParser_GBI1_MoveWord );
UcodeFunc( DLParser_GBI1_Noop );

//*****************************************************************************
// GBI2
//*****************************************************************************

UcodeFunc( DLParser_GBI2_DMA_IO );
UcodeFunc( DLParser_GBI2_MoveWord );
UcodeFunc( DLParser_GBI2_MoveMem );

//*****************************************************************************
// Include ucode header files
//*****************************************************************************

#include "gsp/gspMacros.h"
#include "gsp/gspSprite2D.h"
#include "gsp/gspS2DEX.h"
#include "gsp/gspCustom.h"


//*****************************************************************************
// RDP Commands
//*****************************************************************************

UcodeFunc( DLParser_TexRect );
UcodeFunc( DLParser_TexRectFlip );
UcodeFunc( DLParser_RDPLoadSync );
UcodeFunc( DLParser_RDPPipeSync );
UcodeFunc( DLParser_RDPTileSync );
UcodeFunc( DLParser_RDPFullSync );
UcodeFunc( DLParser_SetKeyGB );
UcodeFunc( DLParser_SetKeyR );
UcodeFunc( DLParser_SetConvert );
UcodeFunc( DLParser_SetScissor );
UcodeFunc( DLParser_SetPrimDepth );
UcodeFunc( DLParser_RDPSetOtherMode );
UcodeFunc( DLParser_LoadTLut );
UcodeFunc( DLParser_SetTileSize );
UcodeFunc( DLParser_LoadBlock );
UcodeFunc( DLParser_LoadTile );
UcodeFunc( DLParser_SetTile );
UcodeFunc( DLParser_FillRect );
UcodeFunc( DLParser_SetFillColor );
UcodeFunc( DLParser_SetFogColor );
UcodeFunc( DLParser_SetBlendColor );
UcodeFunc( DLParser_SetPrimColor );
UcodeFunc( DLParser_SetEnvColor );
UcodeFunc( DLParser_SetCombine );
UcodeFunc( DLParser_SetTImg );
UcodeFunc( DLParser_SetZImg );
UcodeFunc( DLParser_SetCImg );

//*****************************************************************************
// RSP Tri Command
//*****************************************************************************

UcodeFunc( DLParser_TriRSP );

//*****************************************************************************
//
//*****************************************************************************


#endif
