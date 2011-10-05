/*
 Copyright (C) 2010 StrmnNrmn

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include "Ucode.h"
#include "stdafx.h"

//*************************************************************************************
// 
//*************************************************************************************
// This is the multiplier applied to vertex indices
const u32 ucode_stride[] =
{
	10,		// Super Mario 64, Tetrisphere, Demos
	2,		// Mario Kart, Star Fox
	2,		// Zelda, and newer games
	2,		// Yoshi's Story, Pokemon Puzzle League
	5,		// Wave Racer USA
	10,		// Diddy Kong Racing, Gemini, and Mickey
	2,		// Last Legion, Toukon, Toukon 2
	5,		// Shadows of the Empire (SOTE)
	10,		// Golden Eye
	2,		// Conker BFD
	10,		// Perfect Dark
};

//*************************************************************************************
// 
//*************************************************************************************
// This the ucode modifier indices
const u32 ucode_modify[] =
{
	0,		// Modified uCode 0 - RSP SW 2.0D EXT
	0,		// Modified uCode 0 - RSP SW 2.0 Diddy
	1,		// Modified uCode 1 - F3DEX Last Legion
	0,		// Modified uCode 0 - RSP SW 2.0D EXT
	0,		// Modified uCode 0 - RSP SW 2.0X
	2,		// Modified uCode 2:  F3DEXBGxx Conker
	0,		// Modified uCode 0 - Unknown
};

//*************************************************************************************
// 
//*************************************************************************************
const MicroCodeInstruction gInstructionLookup[MAX_UCODE][256] =
{
	// uCode 0 - RSP SW 2.0X
	// Games: Super Mario 64, Tetrisphere, Demos
	{
		DLParser_GBI1_SpNoop, DLParser_GBI1_Mtx, DLParser_GBI1_Reserved, DLParser_GBI1_MoveMem,
		DLParser_GBI0_Vtx, DLParser_GBI1_Reserved, DLParser_GBI1_DL, DLParser_GBI1_Reserved,
		DLParser_GBI1_Reserved, DLParser_GBI1_Sprite2DBase, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		//10
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		//20
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		//30
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		//40
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		//50
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		//60
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		//70
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,

		//80
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		//90
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		//a0
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		//b0
		DLParser_Nothing,DLParser_GBI0_Tri4, DLParser_GBI1_RDPHalf_Cont, DLParser_GBI1_RDPHalf_2,
		DLParser_GBI1_RDPHalf_1, DLParser_GBI1_Line3D, DLParser_GBI1_ClearGeometryMode, DLParser_GBI1_SetGeometryMode,
		DLParser_GBI1_EndDL, DLParser_GBI1_SetOtherModeL, DLParser_GBI1_SetOtherModeH, DLParser_GBI1_Texture,
		DLParser_GBI1_MoveWord, DLParser_GBI1_PopMtx, DLParser_GBI1_CullDL, DLParser_GBI1_Tri1,

		//c0
		DLParser_GBI1_Noop, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_TriRSP, DLParser_TriRSP, DLParser_TriRSP, DLParser_TriRSP,
		DLParser_TriRSP, DLParser_TriRSP, DLParser_TriRSP, DLParser_TriRSP,
		//d0
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		//e0
		DLParser_Nothing, DLParser_Nothing, DLParser_Nothing, DLParser_Nothing,
		DLParser_TexRect, DLParser_TexRectFlip, DLParser_RDPLoadSync, DLParser_RDPPipeSync,
		DLParser_RDPTileSync, DLParser_RDPFullSync, DLParser_SetKeyGB, DLParser_SetKeyR,
		DLParser_SetConvert, DLParser_SetScissor, DLParser_SetPrimDepth, DLParser_RDPSetOtherMode,
		//f0
		DLParser_LoadTLut, DLParser_Nothing, DLParser_SetTileSize, DLParser_LoadBlock, 
		DLParser_LoadTile, DLParser_SetTile, DLParser_FillRect, DLParser_SetFillColor,
		DLParser_SetFogColor, DLParser_SetBlendColor, DLParser_SetPrimColor, DLParser_SetEnvColor,
		DLParser_SetCombine, DLParser_SetTImg, DLParser_SetZImg, DLParser_SetCImg
	},
	// uCode 1 - F3DEX 1.XX
	// 00-3f
	// games: Mario Kart, Star Fox
	{
		DLParser_GBI1_SpNoop,	DLParser_GBI1_Mtx,			DLParser_GBI1_Reserved,		DLParser_GBI1_MoveMem,
		DLParser_GBI1_Vtx,		DLParser_GBI1_Reserved,		DLParser_GBI1_DL,			DLParser_GBI1_Reserved,
		DLParser_GBI1_Reserved, DLParser_GBI1_Sprite2DBase, DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing, 
		//10
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//20
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//30
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//40
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//50
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//60
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//70
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//80
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//90
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//a0
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_GBI1_LoadUCode, 
		//b0
		DLParser_GBI1_BranchZ,	DLParser_GBI1_Tri2,			DLParser_GBI1_ModifyVtx,		DLParser_GBI1_RDPHalf_2,
		DLParser_GBI1_RDPHalf_1,DLParser_GBI1_Line3D,		DLParser_GBI1_ClearGeometryMode,DLParser_GBI1_SetGeometryMode,
		DLParser_GBI1_EndDL,	DLParser_GBI1_SetOtherModeL,DLParser_GBI1_SetOtherModeH,	DLParser_GBI1_Texture,
		DLParser_GBI1_MoveWord, DLParser_GBI1_PopMtx,		DLParser_GBI1_CullDL,			DLParser_GBI1_Tri1,
		//c0
		DLParser_GBI1_Noop,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,    
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,    
		DLParser_TriRSP,		DLParser_TriRSP,			DLParser_TriRSP,			DLParser_TriRSP,
		DLParser_TriRSP,		DLParser_TriRSP,			DLParser_TriRSP,			DLParser_TriRSP,
		//d0
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//e0
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing, 
		DLParser_TexRect,		DLParser_TexRectFlip,		DLParser_RDPLoadSync,		DLParser_RDPPipeSync,
		DLParser_RDPTileSync,	DLParser_RDPFullSync,		DLParser_SetKeyGB,			DLParser_SetKeyR,
		DLParser_SetConvert,	DLParser_SetScissor,		DLParser_SetPrimDepth,		DLParser_RDPSetOtherMode,
		//f0
		DLParser_LoadTLut,		DLParser_Nothing,			DLParser_SetTileSize,		DLParser_LoadBlock, 
		DLParser_LoadTile,		DLParser_SetTile,			DLParser_FillRect,			DLParser_SetFillColor,
		DLParser_SetFogColor,	DLParser_SetBlendColor,		DLParser_SetPrimColor,		DLParser_SetEnvColor,
		DLParser_SetCombine,	DLParser_SetTImg,			DLParser_SetZImg,			DLParser_SetCImg
	},

	// Ucode:F3DEX_GBI_2
	// Zelda and new games
	{
		DLParser_GBI1_SpNoop,	DLParser_GBI2_Vtx,			DLParser_GBI1_ModifyVtx,	DLParser_GBI1_CullDL,
		DLParser_GBI1_BranchZ,  DLParser_GBI2_Tri1,			DLParser_GBI2_Tri2,         DLParser_GBI2_Quad,
		DLParser_GBI2_Line3D,   DLParser_S2DEX_Bg1cyc,		DLParser_S2DEX_BgCopy,		DLParser_S2DEX_ObjRendermode,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//10
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//20
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//30
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//40
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//50
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//60
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//70
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//80
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//90
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//a0
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//b0
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		//c0
		DLParser_GBI1_Noop,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,    
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,    
		DLParser_TriRSP,		DLParser_TriRSP,			DLParser_TriRSP,			DLParser_TriRSP,
		DLParser_TriRSP,		DLParser_TriRSP,			DLParser_TriRSP,			DLParser_TriRSP,
		//d0
		DLParser_Nothing,		DLParser_Nothing,			DLParser_Nothing,			DLParser_Nothing,
		DLParser_Nothing,		DLParser_GBI2_DL_Count,		DLParser_GBI2_DMA_IO,		DLParser_GBI2_Texture,
		DLParser_GBI2_PopMtx,	DLParser_GBI2_GeometryMode, DLParser_GBI2_Mtx,			DLParser_GBI2_MoveWord,
		DLParser_GBI2_MoveMem,	DLParser_GBI2_LoadUCode,	DLParser_GBI1_DL,			DLParser_GBI1_EndDL,
		//e0
		DLParser_GBI1_SpNoop,	DLParser_GBI1_RDPHalf_1,	DLParser_GBI2_SetOtherModeL,DLParser_GBI2_SetOtherModeH,
		DLParser_TexRect,		DLParser_TexRectFlip,		DLParser_RDPLoadSync,		DLParser_RDPPipeSync,
		DLParser_RDPTileSync,	DLParser_RDPFullSync,		DLParser_SetKeyGB,			DLParser_SetKeyR,
		DLParser_SetConvert,	DLParser_SetScissor,		DLParser_SetPrimDepth,		DLParser_RDPSetOtherMode,
		//f0
		DLParser_LoadTLut,		DLParser_GBI1_RDPHalf_2,	DLParser_SetTileSize,		DLParser_LoadBlock, 
		DLParser_LoadTile,		DLParser_SetTile,			DLParser_FillRect,			DLParser_SetFillColor,
		DLParser_SetFogColor,	DLParser_SetBlendColor,		DLParser_SetPrimColor,		DLParser_SetEnvColor,
		DLParser_SetCombine,	DLParser_SetTImg,			DLParser_SetZImg,			DLParser_SetCImg
	},

	// Ucode: S2DEX 1.--
	// Games: Yoshi's Story
	{
		DLParser_GBI1_SpNoop,		DLParser_S2DEX_Bg1cyc_2,		DLParser_S2DEX_BgCopy,	DLParser_S2DEX_ObjRectangle,
		DLParser_S2DEX_ObjSprite,	DLParser_S2DEX_ObjMoveMem,		DLParser_GBI1_DL,		DLParser_GBI1_Reserved,
		DLParser_GBI1_Reserved,		DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		//10
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		//20
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		//30
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		//40
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		//50
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		//60
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		//70
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		//80
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		//90
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		//a0
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_GBI1_LoadUCode,
		//b0
		DLParser_S2DEX_SelectDl,	DLParser_S2DEX_ObjRendermode,	DLParser_S2DEX_ObjRectangleR,	DLParser_GBI1_RDPHalf_2,
		DLParser_GBI1_RDPHalf_1,	DLParser_GBI1_Line3D,			DLParser_GBI1_ClearGeometryMode,DLParser_GBI1_SetGeometryMode,
		DLParser_GBI1_EndDL,		DLParser_GBI1_SetOtherModeL,	DLParser_GBI1_SetOtherModeH,	DLParser_GBI1_Texture,
		DLParser_GBI1_MoveWord,		DLParser_GBI1_PopMtx,			DLParser_GBI1_CullDL,			DLParser_GBI1_Tri1,
		//c0
		DLParser_GBI1_Noop,			DLParser_S2DEX_ObjLoadTxtr,		DLParser_S2DEX_ObjLdtxSprite,	DLParser_S2DEX_ObjLdtxRect,
		DLParser_S2DEX_ObjLdtxRectR,DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_TriRSP,			DLParser_TriRSP,				DLParser_TriRSP,		DLParser_TriRSP,
		DLParser_TriRSP,			DLParser_TriRSP,				DLParser_TriRSP,		DLParser_TriRSP,
		//d0
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		//e0
		DLParser_Nothing,			DLParser_Nothing,				DLParser_Nothing,		DLParser_Nothing,
		DLParser_S2DEX_RDPHalf_0,	DLParser_TexRectFlip,			DLParser_RDPLoadSync,	DLParser_RDPPipeSync,
		DLParser_RDPTileSync,		DLParser_RDPFullSync,			DLParser_SetKeyGB,		DLParser_SetKeyR,
		DLParser_SetConvert,		DLParser_SetScissor,			DLParser_SetPrimDepth,	DLParser_RDPSetOtherMode,
		//f0
		DLParser_LoadTLut,			DLParser_Nothing,				DLParser_SetTileSize,	DLParser_LoadBlock, 
		DLParser_LoadTile,			DLParser_SetTile,				DLParser_FillRect,		DLParser_SetFillColor,
		DLParser_SetFogColor,		DLParser_SetBlendColor,			DLParser_SetPrimColor,	DLParser_SetEnvColor,
		DLParser_SetCombine,		DLParser_SetTImg,				DLParser_SetZImg,		DLParser_SetCImg
	},
};

//*************************************************************************************
// 
//*************************************************************************************
#if defined(DAEDALUS_DEBUG_DISPLAYLIST) || defined(DAEDALUS_ENABLE_PROFILING)
const char * gInstructionName[MAX_UCODE][256] =
{
	// uCode 0 - RSP SW 2.0X
	// Games: Super Mario 64, Tetrisphere, Demos
	{
		"G_GBI1_SpNoop", "G_GBI1_Mtx", "G_GBI1_Reserved", "G_GBI1_MoveMem",
		"G_GBI0_Vtx", "G_GBI1_Reserved", "G_GBI1_DL", "G_GBI1_Reserved",
		"G_GBI1_Reserved", "G_GBI1_Sprite2DBase", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//10
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//20
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//30
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//40
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//50
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//60
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//70
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",

		//80
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//90
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//a0
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//b0
		"G_Nothing", "G_GBI0_Tri4", "G_GBI1_RDPHalf_Cont", "G_GBI1_RDPHalf_2",
		"G_GBI1_RDPHalf_1", "G_GBI1_Line3D", "G_GBI1_ClearGeometryMode", "G_GBI1_SetGeometryMode",
		"G_GBI1_EndDL", "G_GBI1_SetOtherModeL", "G_GBI1_SetOtherModeH", "G_GBI1_Texture",
		"G_GBI1_MoveWord", "G_GBI1_PopMtx", "G_GBI1_CullDL", "G_GBI1_Tri1",

		//c0
		"G_GBI1_Noop", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_TriRSP", "G_TriRSP", "G_TriRSP", "G_TriRSP",
		"G_TriRSP", "G_TriRSP", "G_TriRSP", "G_TriRSP",
		//d0
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//e0
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_TexRect", "G_TexRectFlip", "G_RDPLoadSync", "G_RDPPipeSync",
		"G_RDPTileSync", "G_RDPFullSync", "G_SetKeyGB", "G_SetKeyR",
		"G_SetConvert", "G_SetScissor", "G_SetPrimDepth", "G_RDPSetOtherMode",
		//f0
		"G_LoadTLut", "G_Nothing", "G_SetTileSize", "G_LoadBlock", 
		"G_LoadTile", "G_SetTile", "G_FillRect", "G_SetFillColor",
		"G_SetFogColor", "G_SetBlendColor", "G_SetPrimColor", "G_SetEnvColor",
		"G_SetCombine", "G_SetTImg", "G_SetZImg", "G_SetCImg"
	},
	//uCode "1 "- "F3DEX "1."XX
	// 00-3f
	//games": "Mario "Kart", "Star "Fox
	{
		"G_GBI1_SpNoop", "G_GBI1_Mtx", "G_GBI1_Reserved", "G_GBI1_MoveMem",
		"G_GBI1_Vtx", "G_GBI1_Reserved", "G_GBI1_DL", "G_GBI1_Reserved",
		"G_GBI1_Reserved", "G_GBI1_Sprite2DBase", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		//40"-"7f": "unused
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		//80"-"bf": "Immediate "commands
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_GBI1_LoadUCode", 
		"G_GBI1_BranchZ", "G_GBI1_Tri2", "G_GBI1_ModifyVtx", "G_GBI1_RDPHalf_2",
		"G_GBI1_RDPHalf_1", "G_GBI1_Line3D", "G_GBI1_ClearGeometryMode", "G_GBI1_SetGeometryMode",
		"G_GBI1_EndDL", "G_GBI1_SetOtherModeL", "G_GBI1_SetOtherModeH", "G_GBI1_Texture",
		"G_GBI1_MoveWord", "G_GBI1_PopMtx", "G_GBI1_CullDL", "G_GBI1_Tri1",
		//c0"-"ff": "RDP "commands
		"G_GBI1_Noop", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_TriRSP", "G_TriRSP", "G_TriRSP", "G_TriRSP",
		"G_TriRSP", "G_TriRSP", "G_TriRSP", "G_TriRSP",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_TexRect", "G_TexRectFlip", "G_RDPLoadSync", "G_RDPPipeSync",
		"G_RDPTileSync", "G_RDPFullSync", "G_SetKeyGB", "G_SetKeyR",
		"G_SetConvert", "G_SetScissor", "G_SetPrimDepth", "G_RDPSetOtherMode",
		"G_LoadTLut", "G_Nothing", "G_SetTileSize", "G_LoadBlock", 
		"G_LoadTile", "G_SetTile", "G_FillRect", "G_SetFillColor",
		"G_SetFogColor", "G_SetBlendColor", "G_SetPrimColor", "G_SetEnvColor",
		"G_SetCombine", "G_SetTImg", "G_SetZImg", "G_SetCImg"
	},

	//Ucode":"F3DEX_GBI_2
	//Zelda "and "new games
	{
		"G_GBI1_Noop", "G_GBI2_Vtx", "G_GBI1_ModifyVtx", "G_GBI2_CullDL",
		"G_GBI1_BranchZ", "G_GBI2_Tri1", "G_GBI2_Tri2", "G_GBI2_Quad",
		"G_GBI2_Line3D", "G_S2DEX_Bg1cyc", "G_S2DEX_BgCopy", "G_S2DEX_ObjRendermode",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//10
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//20
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//30
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//40
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//50
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//60
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//70
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",

		//80
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//90
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		//a0
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_GBI1_LoadUCode",
		//b0
		"G_GBI1_BranchZ", "G_GBI0_Tri4", "G_GBI1_ModifyVtx", "G_GBI1_RDPHalf_2",
		"G_GBI1_RDPHalf_1", "G_GBI1_Line3D", "G_GBI1_ClearGeometryMode", "G_GBI1_SetGeometryMode",
		"G_GBI1_EndDL", "G_GBI1_SetOtherModeL", "G_GBI1_SetOtherModeH", "G_GBI1_Texture",
		"G_GBI1_MoveWord", "G_GBI1_PopMtx", "G_GBI1_CullDL", "G_GBI1_Tri1",
		//c0
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_TriRSP", "G_TriRSP", "G_TriRSP", "G_TriRSP",
		"G_TriRSP", "G_TriRSP", "G_TriRSP", "G_TriRSP",
		//d0
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_GBI2_DL_Count", "G_GBI2_DMA_IO", "G_GBI2_Texture",
		"G_GBI2_PopMtx", "G_GBI2_GeometryMode", "G_GBI2_Mtx", "G_GBI2_MoveWord",
		"G_GBI2_MoveMem", "G_GBI1_LoadUCode", "G_GBI2_DL", "G_GBI2_EndDL",
		//e0
		"G_GBI1_SpNoop", "G_GBI1_RDPHalf_1", "G_GBI2_SetOtherModeL", "G_GBI2_SetOtherModeH",
		"G_TexRect", "G_TexRectFlip", "G_RDPLoadSync", "G_RDPPipeSync",
		"G_RDPTileSync", "G_RDPFullSync", "G_SetKeyGB", "G_SetKeyR",
		"G_SetConvert", "G_SetScissor", "G_SetPrimDepth", "G_RDPSetOtherMode",
		//f0
		"G_LoadTLut", "G_Nothing", "G_SetTileSize", "G_LoadBlock", 
		"G_LoadTile", "G_SetTile", "G_FillRect", "G_SetFillColor",
		"G_SetFogColor", "G_SetBlendColor", "G_SetPrimColor", "G_SetEnvColor",
		"G_SetCombine", "G_SetTImg", "G_SetZImg", "G_SetCImg"
	},

	// Ucode: S2DEX 1.--
	// Games: Yoshi's Story
	{
		"G_GBI1_SpNoop", "G__S2DEX_Bg1cyc_2", "G_S2DEX_BgCopy", "G_S2DEX_ObjRectangle",
		"G_S2DEX_ObjSprite", "G_S2DEX_ObjMoveMem", "G_GBI1_DL", "G_GBI1_Reserved",
		"G_GBI1_Reserved", "G_GBI1_Sprite2DBase", "G_Nothing", "G_Nothing",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		//40"-"7f": "unused
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		//80"-"bf": "Immediate "commands
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_GBI1_LoadUCode", 
		"G_S2DEX_SelectDl",	"G_S2DEX_ObjRendermode",	"G_S2DEX_ObjRectangleR",	"G_GBI1_RDPHalf_2",
		"G_GBI1_RDPHalf_1", "G_GBI1_Line3D", "G_GBI1_ClearGeometryMode", "G_GBI1_SetGeometryMode",
		"G_GBI1_EndDL", "G_GBI1_SetOtherModeL", "G_GBI1_SetOtherModeH", "G_GBI1_Texture",
		"G_GBI1_MoveWord", "G_GBI1_PopMtx", "G_GBI1_CullDL", "G_GBI1_Tri1",
		//c0
		"G_GBI1_Noop",			"G_S2DEX_ObjLoadTxtr",		"G_S2DEX_ObjLdtxSprite",	"G_S2DEX_ObjLdtxRect",
		"G_S2DEX_ObjLdtxRectR","G_Nothing",				"G_Nothing",		"G_Nothing",
		"G_TriRSP", "G_TriRSP", "G_TriRSP", "G_TriRSP",
		"G_TriRSP", "G_TriRSP", "G_TriRSP", "G_TriRSP",
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_Nothing", "G_Nothing", "G_Nothing", "G_Nothing", 
		"G_TexRect", "G_TexRectFlip", "G_RDPLoadSync", "G_RDPPipeSync",
		"G_RDPTileSync", "G_RDPFullSync", "G_SetKeyGB", "G_SetKeyR",
		"G_SetConvert", "G_SetScissor", "G_SetPrimDepth", "G_RDPSetOtherMode",
		"G_LoadTLut", "G_Nothing", "G_SetTileSize", "G_LoadBlock", 
		"G_LoadTile", "G_SetTile", "G_FillRect", "G_SetFillColor",
		"G_SetFogColor", "G_SetBlendColor", "G_SetPrimColor", "G_SetEnvColor",
		"G_SetCombine", "G_SetTImg", "G_SetZImg", "G_SetCImg"
	}
};
#endif
