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
#ifndef HLEGRAPHICS_UCODES_UCODE_H_
#define HLEGRAPHICS_UCODES_UCODE_H_

#include "stdafx.h"
#include "HLEGraphics/uCodes/UcodeDefs.h"

typedef void(*MicroCodeInstruction)(MicroCodeCommand);
#define UcodeFunc(name)	void name(MicroCodeCommand)

extern const MicroCodeInstruction gNormalInstruction[5][256];
extern const char * gNormalInstructionName[5][256];

template< u32 VertexStride > 
void DLParser_GBI1_Tri1_T( MicroCodeCommand command );
template< u32 VertexStride > 
void DLParser_GBI1_Tri2_T( MicroCodeCommand command );
template< u32 VertexStride > 
void DLParser_GBI1_Line3D_T( MicroCodeCommand command );

UcodeFunc( DLParser_GBI0_Vtx );
UcodeFunc( DLParser_GBI0_CullDL );
UcodeFunc( DLParser_GBI0_Tri1 );
UcodeFunc( DLParser_GBI0_Line3D );
UcodeFunc( DLParser_GBI0_Tri4 );

UcodeFunc( DLParser_GBI1_CullDL );
UcodeFunc( DLParser_GBI1_DL );
UcodeFunc( DLParser_GBI1_BranchZ );
UcodeFunc( DLParser_GBI1_LoadUCode );

UcodeFunc( DLParser_GBI1_GeometryMode );
UcodeFunc( DLParser_GBI2_GeometryMode );
UcodeFunc( DLParser_GBI1_SetOtherModeL );
UcodeFunc( DLParser_GBI1_SetOtherModeH );
UcodeFunc( DLParser_GBI2_SetOtherModeL );
UcodeFunc( DLParser_GBI2_SetOtherModeH );

UcodeFunc( DLParser_GBI1_Texture );
UcodeFunc( DLParser_GBI2_Texture );

UcodeFunc( DLParser_GBI1_Vtx );
UcodeFunc( DLParser_GBI2_Vtx );
UcodeFunc( DLParser_GBI1_ModifyVtx );

UcodeFunc( DLParser_GBI1_Mtx );
UcodeFunc( DLParser_GBI2_Mtx );
UcodeFunc( DLParser_GBI1_PopMtx );

UcodeFunc( DLParser_GBI2_Quad );
UcodeFunc( DLParser_GBI2_Line3D );
UcodeFunc( DLParser_GBI2_Tri1 );
UcodeFunc( DLParser_GBI2_Tri2 );

UcodeFunc( DLParser_GBI1_Tri1 );
UcodeFunc( DLParser_GBI1_Tri2 );
UcodeFunc( DLParser_GBI1_Line3D );

//*****************************************************************************
// New GBI2 ucodes
//*****************************************************************************
UcodeFunc( DLParser_GBI2_DL_Count );

//*****************************************************************************
// GBI1
//*****************************************************************************

UcodeFunc( DLParser_Nothing);
UcodeFunc( DLParser_GBI1_EndDL );
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
UcodeFunc( DLParser_GBI2_PopMtx );

//*****************************************************************************
// Sprite2D
//*****************************************************************************

UcodeFunc( DLParser_GBI1_Sprite2DBase );

//*****************************************************************************
// S2DEX
//*****************************************************************************

UcodeFunc( DLParser_S2DEX_BgCopy );
UcodeFunc( DLParser_S2DEX_SelectDl );
UcodeFunc( DLParser_S2DEX_ObjSprite );
UcodeFunc( DLParser_S2DEX_ObjRectangle );
UcodeFunc( DLParser_S2DEX_ObjRendermode );
UcodeFunc( DLParser_S2DEX_ObjLoadTxtr );
UcodeFunc( DLParser_S2DEX_ObjLdtxSprite );
UcodeFunc( DLParser_S2DEX_ObjLdtxRect );
UcodeFunc( DLParser_S2DEX_ObjLdtxRectR );
UcodeFunc( DLParser_S2DEX_RDPHalf_0 );
UcodeFunc( DLParser_S2DEX_ObjMoveMem );
UcodeFunc( DLParser_S2DEX_Bg1cyc );
UcodeFunc( DLParser_S2DEX_ObjRectangleR );

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
// RSP Tri Command (LLE)
//*****************************************************************************
UcodeFunc( DLParser_TriRSP );

//*****************************************************************************
// Custom
//*****************************************************************************
UcodeFunc( DLParser_GBI0_Vtx_Beta );
UcodeFunc( DLParser_GBI0_Line3D_Beta );
UcodeFunc( DLParser_GBI0_Tri1_Beta );
UcodeFunc( DLParser_GBI0_Tri2_Beta );
UcodeFunc( DLParser_Last_Legion_0x80 );
UcodeFunc( DLParser_Last_Legion_0x00 );
UcodeFunc( DLParser_TexRect_Last_Legion );
UcodeFunc( DLParser_DLInMem );
UcodeFunc( DLParser_Mtx_DKR );
UcodeFunc( DLParser_MoveWord_DKR );
UcodeFunc( DLParser_Set_Addr_DKR );
UcodeFunc( DLParser_GBI1_Texture_DKR );
UcodeFunc( DLParser_GBI0_Vtx_DKR );
UcodeFunc( DLParser_DMA_Tri_DKR );
UcodeFunc( DLParser_Tri1_Conker );
UcodeFunc( DLParser_Tri2_Conker );
UcodeFunc( DLParser_Tri4_Conker );
UcodeFunc( DLParser_MoveMem_Conker );
UcodeFunc( DLParser_MoveWord_Conker );
UcodeFunc( DLParser_Vtx_Conker );
UcodeFunc( DLParser_Set_Vtx_CI_PD );
UcodeFunc( DLParser_Vtx_PD );
UcodeFunc( DLParser_Trix_GE );
UcodeFunc( DLParser_RDPHalf1_GE );

#endif // HLEGRAPHICS_UCODES_UCODE_H___
