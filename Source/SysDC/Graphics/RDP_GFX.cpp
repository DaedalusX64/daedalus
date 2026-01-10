/*
Copyright (C) 2001 StrmnNrmn

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

#include "stdafx.h"

#include "TextureHandler.h"
#include "ConvertImage.h"
#include "Core/CPU.h"			// AddCPUJob(CPU_CHECK_INTERRUPTS)
#include "PVRRender.h"
#include "Debug/MyDebug.h"
#include "Debug/Dump.h"
#include "ROM.h"			// ROM_Check_Ucode
#include "RDP.h"
#include "RDP_GFX.h"
#include "core/memory.h"


#include "daedalus_crc.h"
#include "ultra_gbi.h"
#include "PngUtil.h"

// Lkb added mirroring support for cards that don't support it. Thanks Lkb!
/*
Description:
	N64 games often use texture mirroring to display semi-transparent circles.
	Examples:
	- Shadow under Mario and other characters
	- Transition that happens when Mario opens the castle door
	- The cannon in Mario
	- The telescope in Majora's Mask (seen in TR64)

	However, some 3D cards do not support texture mirroring and mirrored textures are displayed incorrectly.
	For the first two cases, this is not a big problem, but it's annoying in the last two.

	This patch emulates mirroring in software when the 3D card doesn't support it (eg. Matrox G400)

	Problems:
	- This patch doesn't redefine any texture coordinate (maybe it's correct, maybe no, I failed to understand this :) )
	- This patch always assumes 2x2 mirroring
	- This patch computes the mirroring at every frame and does not cache the texture
*/

static void RDP_EnableTexturing(u32 dwTile);
static void RDP_InitRenderState();
static void RDP_GFX_DumpVtxInfo(u32 dwAddress, u32 dwV0, u32 dwN);
static void RDP_GFX_DumpVtxInfoDKR(u32 dwAddress, u32 dwV0, u32 dwN);
static void RDP_GFX_DumpScreenshot();

static u32 g_dwNumTrisRendered;
static u32 g_dwNumDListsCulled;
static u32 g_dwNumTexturesIgnored;
static u32 g_dwNumTrisClipped;
static u32 g_dwNumVertices;


static BOOL g_bDetermineduCode = FALSE;
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                    uCode Config                      //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

// This is the multiplier applied to vertex indices. 
// For Mario 64, it is 10.
// For Starfox, Mariokart etc it is 2.
static u32 g_dwVertexMult = 10;

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                      Dumping                         //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
static BOOL g_bDropAllTextures = FALSE;


//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                     GFX State                        //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
static PVRRender * g_Renderer;



typedef struct _SetTImgInfo
{
	u32 dwFormat;
	u32 dwSize;
	u32 dwWidth;
	u32 dwAddr;
} SetImgInfo;


static u32	g_dwSegment[16];
static N64Light  g_N64Lights[8];
static Tile g_Tiles[8];
static SetImgInfo g_TI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };
static SetImgInfo g_CI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };
static SetImgInfo g_DI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };

// The display list PC stack. Before this was an array of 10
// items, but this way we can nest as deeply as necessary. 

typedef struct 
{
	u32 addr;
	u32 limit;
	// Push/pop?
} DList;

static std::vector< DList > g_dwPCStack;

static BOOL  g_bTextureEnable = FALSE;
static u32 g_dwGeometryMode = 0;
static u32 g_dwOtherModeL   = 0;
static u32 g_dwOtherModeH   = 0;

enum RGDGFX_LOAD_TYPE
{
	LOAD_BLOCK = 0,
	LOAD_TILE
};
static RGDGFX_LOAD_TYPE g_nLastLoad = LOAD_BLOCK;
static u32 g_dwLoadTilePitch;				// Pitch (as specified at time of LoadTile)
static u32 g_dwLoadAddress;				// Address (as specified at LoadBlock)

static u32 g_dwPalAddress = 0;
static u32 g_dwPalSize = 0;

static u32 g_dwAmbientLight = 0;

static u32 g_dwTextureTile = 0;

static struct
{
	u32 x0, y0, x1, y1, mode;
} g_Scissor;

static BOOL g_bDXTZero = FALSE;					// HACK - need to pass this directly to TH!


static u32 g_dwFillColor		= 0xFFFFFFFF;




static u32 g_dwLastPurgeTimeTime = 0;		// Time textures were last purged

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                      Strings                         //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

static const char * sc_colcombtypes32[32] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ", 
	"Shade       ", "Env         ",
	"1           ", "CombAlp     ",
	"Texel0_Alp  ", "Texel1_Alp  ",
	"Prim_Alpha  ", "Shade_Alpha ",
	"Env_Alpha   ", "LOD_Frac    ",
	"PrimLODFrac ", "K5          ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ",	"0           "
};
static const char *sc_colcombtypes16[16] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ", 
	"Shade       ", "Env         ",
	"1           ", "CombAlp     ",
	"Texel0_Alp  ", "Texel1_Alp  ",
	"Prim_Alp    ", "Shade_Alpha ",
	"Env_Alpha   ", "LOD_Frac    ",
	"PrimLOD_Frac", "0           "
};
static const char *sc_colcombtypes8[8] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ", 
	"Shade       ", "Env         ",
	"1           ", "0           ",
};



// Mask down to 0x003FFFFF?
#define RDPSegAddr(seg) ( g_dwSegment[(seg>>24)&0x0F] + (seg&0x00FFFFFF) )


static void RDP_GFX_Nothing(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SPNOOP(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_Mtx(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_MtxDKR(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaMoveWordTri(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_MoveMem(u32 dwCmd0, u32 dwCmd1);

static void RDP_GFX_Vtx_Mario(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_Vtx_DKR(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_Vtx_MarioKart(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_Vtx_WaveraceUS(u32 dwCmd0, u32 dwCmd1);

static void RDP_GFX_DmaTri(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_DL(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_DLInMem(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_Reserved3(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_Sprite2D(u32 dwCmd0, u32 dwCmd1);

static void RDP_GFX_Tri2_Mariokart(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_Tri2_Goldeneye(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_RDPHalf_Cont(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_RDPHalf_2(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_RDPHalf_1(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_Line3D(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ClearGeometryMode(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetGeometryMode(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_EndDL(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetOtherMode_L(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetOtherMode_H(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_Texture(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_MoveWord(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_PopMtx(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_CullDL(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_Tri1(u32 dwCmd0, u32 dwCmd1);

static void RDP_GFX_NOOP(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_TexRect(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_TexRectFlip(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_RDPLoadSync(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_RDPPipeSync(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_RDPTileSync(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_RDPFullSync(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetKeyGB(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetKeyR(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetConvert(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetScissor(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetPrimDepth(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_RDPSetOtherMode(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_LoadTLut(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetTileSize(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_LoadBlock(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_LoadTile(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetTile(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_FillRect(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetFillColor(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetFogColor(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetBlendColor(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetPrimColor(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetEnvColor(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetCombine(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetTImg(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetZImg(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_SetCImg(u32 dwCmd0, u32 dwCmd1);

static void RDP_GFX_AST_Unk1(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_YS_Unk1(u32 dwCmd0, u32 dwCmd1);

static void RDP_GFX_ZeldaDL(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaCullDL(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaEndDL(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaMoveWord(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaTexture(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaSetGeometryMode(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaSetOtherMode_L(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaSetOtherMode_H(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaMoveMem(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaMtx(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaPopMtx(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_Vtx_Zelda(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaTri2(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaTri3(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaTri1(u32 dwCmd0, u32 dwCmd1);


static void RDP_GFX_AeroUnk1(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_Zelda2Unk1(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_Zelda2Unk2(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaUnk1(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaUnk2(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaUnk3(u32 dwCmd0, u32 dwCmd1);
static void RDP_GFX_ZeldaUnk4(u32 dwCmd0, u32 dwCmd1);

static LPCSTR g_szRDPInstrName[256] =
{
	//               ZELDA_LOADVTX                            ZELDA_CULLDL
	"G_SPNOOP",	 "G_MTX",     "G_ZMOVEWORDTRI", "G_MOVEMEM",
	//   ZELDAUNK4           ZELDATRI1      ZELDATRI2   ZELDATRI3
	"G_VTX", "G_DMATRI", "G_DL",    "G_DLINMEM",
	"G_RESERVED3", "G_SPRITE2D", "G_ZUNK3", "G_Z2UNK2",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
//10
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
//20
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
//30
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
//40
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
//50
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
//60
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
//70
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",

//80
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
//90
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
//A0
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_AST_UNK1",
//B0
	"G_AEROUNK1", "G_TRI2",    "G_RDPHALF_CONT", "G_RDPHALF_2",
	"G_RDPHALF_1", "G_LINE3D", "G_CLEARGEOMETRYMODE", "G_SETGEOMETRYMODE",
	"G_ENDDL", "G_SETOTHERMODE_L", "G_SETOTHERMODE_H", "G_TEXTURE",
	"G_MOVEWORD", "G_POPMTX", "G_CULLDL", "G_TRI1",

//C0
	"G_NOOP",    "G_NOTHING", "G_YS_UNK1", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
//D0
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_NOTHING",
	"G_NOTHING", "G_NOTHING", "G_NOTHING", "G_ZTEXTURE",
	"G_ZPOPMTX", "G_ZSETGEOMETRYMODE", "G_ZMTX", "G_ZMOVEWORD",
	"G_ZMOVEMEM", "G_ZUNK2", "G_ZDL", "G_ZENDDL",
//E0
	"G_Z2UNK1", "G_ZUNK1", "G_ZSETOTHERMODE_L", "G_ZSETOTHERMODE_H",
	"G_TEXRECT", "G_TEXRECTFLIP", "G_RDPLOADSYNC", "G_RDPPIPESYNC",
	"G_RDPTILESYNC", "G_RDPFULLSYNC", "G_SETKEYGB", "G_SETKEYR",
	"G_SETCONVERT", "G_SETSCISSOR", "G_SETPRIMDEPTH", "G_RDPSETOTHERMODE",
//F0
	"G_LOADTLUT", "G_NOTHING", "G_SETTILESIZE", "G_LOADBLOCK", 
	"G_LOADTILE", "G_SETTILE", "G_FILLRECT", "G_SETFILLCOLOR",
	"G_SETFOGCOLOR", "G_SETBLENDCOLOR", "G_SETPRIMCOLOR", "G_SETENVCOLOR",
	"G_SETCOMBINE", "G_SETTIMG", "G_SETZIMG", "G_SETCIMG"


};

static RDPInstruction GFXInstruction[256] =
{
	// The First 128 Instructions (0..127) are "DMA" commands (i.e. they
	//  require a DMA transfer to move the required data info position)
	//               Zelda_LoadVtx                            Zelda_CullDL
	RDP_GFX_SPNOOP,	 RDP_GFX_Mtx,     RDP_GFX_ZeldaMoveWordTri, RDP_GFX_MoveMem,
	//   ZeldaUnk4           ZeldaTri1      ZeldaTri2   ZeldaTri3
	RDP_GFX_Vtx_Mario, RDP_GFX_DmaTri, RDP_GFX_DL,    RDP_GFX_DLInMem,
	RDP_GFX_Reserved3, RDP_GFX_Sprite2D, RDP_GFX_ZeldaUnk3, RDP_GFX_Zelda2Unk2,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
//10
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
//20
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
//30
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
//40
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
//50
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
//60
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
//70
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,

	// The next 64 commands (-65 .. -128, or 128..191) are "Immediate" commands,
	//  in that they can be executed immediately with no further memory transfers
//80
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
//90
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
//a0
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_AST_Unk1,
//b0
	RDP_GFX_AeroUnk1, RDP_GFX_Tri2_Mariokart,    RDP_GFX_RDPHalf_Cont, RDP_GFX_RDPHalf_2,
	RDP_GFX_RDPHalf_1, RDP_GFX_Line3D, RDP_GFX_ClearGeometryMode, RDP_GFX_SetGeometryMode,
	RDP_GFX_EndDL, RDP_GFX_SetOtherMode_L, RDP_GFX_SetOtherMode_H, RDP_GFX_Texture,
	RDP_GFX_MoveWord, RDP_GFX_PopMtx, RDP_GFX_CullDL, RDP_GFX_Tri1,

	// The last 64 commands are "RDP" commands; they are passed through the
	//  RSP and sent to the RDP directly.
//c0
	RDP_GFX_NOOP,    RDP_GFX_Nothing, RDP_GFX_YS_Unk1, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
//d0
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing,
	RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_Nothing, RDP_GFX_ZeldaTexture,
	RDP_GFX_ZeldaPopMtx, RDP_GFX_ZeldaSetGeometryMode, RDP_GFX_ZeldaMtx, RDP_GFX_ZeldaMoveWord,
	RDP_GFX_ZeldaMoveMem, RDP_GFX_ZeldaUnk2, RDP_GFX_ZeldaDL, RDP_GFX_ZeldaEndDL,
//e0
	RDP_GFX_Zelda2Unk1, RDP_GFX_ZeldaUnk1, RDP_GFX_ZeldaSetOtherMode_L, RDP_GFX_ZeldaSetOtherMode_H,
	RDP_GFX_TexRect, RDP_GFX_TexRectFlip, RDP_GFX_RDPLoadSync, RDP_GFX_RDPPipeSync,
	RDP_GFX_RDPTileSync, RDP_GFX_RDPFullSync, RDP_GFX_SetKeyGB, RDP_GFX_SetKeyR,
	RDP_GFX_SetConvert, RDP_GFX_SetScissor, RDP_GFX_SetPrimDepth, RDP_GFX_RDPSetOtherMode,
//f0
	RDP_GFX_LoadTLut, RDP_GFX_Nothing, RDP_GFX_SetTileSize, RDP_GFX_LoadBlock, 
	RDP_GFX_LoadTile, RDP_GFX_SetTile, RDP_GFX_FillRect, RDP_GFX_SetFillColor,
	RDP_GFX_SetFogColor, RDP_GFX_SetBlendColor, RDP_GFX_SetPrimColor, RDP_GFX_SetEnvColor,
	RDP_GFX_SetCombine, RDP_GFX_SetTImg, RDP_GFX_SetZImg, RDP_GFX_SetCImg
};

//DKR: 00229BA8: 05710080 001E4AF0 CMD G_DMATRI  Triangles 9 at 801E4AF0


HRESULT RDP_GFX_Init()
{
	HRESULT hr;

	g_bDetermineduCode = FALSE;
	g_dwLastPurgeTimeTime = g_dwRDPTime;

	hr = S_OK;

	g_Renderer = new PVRRender();
	if (g_Renderer == NULL)
		return E_OUTOFMEMORY;

	if (SUCCEEDED(hr))
		hr = g_Renderer->Initialize();

	if (FAILED(hr))
		return hr;

	if (!TH_InitTable(8000))
		return E_FAIL;


	return S_OK;
}


HRESULT RDP_GFX_Reset()
{
	g_dwPCStack.clear();

	g_bDetermineduCode = FALSE;

	TH_DropTextures();

	return S_OK;
}


void RDP_GFX_Cleanup()
{
	TH_DestroyTable();

	if (g_Renderer != NULL)
	{
		delete g_Renderer;
		g_Renderer = NULL;
	}
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                      Logging                         //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
void RDP_GFX_DropTextures()
{
	g_bDropAllTextures = TRUE;
}

void RDP_GFX_MakeTexturesBlue()
{
	g_bTHMakeTexturesBlue = TRUE;
	RDP_GFX_DropTextures();
}
void RDP_GFX_MakeTexturesNormal()
{
	g_bTHMakeTexturesBlue = FALSE;
	RDP_GFX_DropTextures();
}

void RDP_GFX_DumpTextures()
{
	DBGConsole_Msg(0, "Texture dumping enabled");
	g_bTHDumpTextures = TRUE;
	RDP_GFX_DropTextures();		// Drop textures so that we can re-create
}
void RDP_GFX_NoDumpTextures()
{
	DBGConsole_Msg(0, "Texture dumping disabled");
	g_bTHDumpTextures = FALSE;
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                   Task Handling                      //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

#define G_ZELDAVTX  G_MTX			// 1
#define G_ZELDACULLDL  G_MOVEMEM	// 3
// 4 is something like a conditional DL
#define G_DMATRI	0x05
#define G_ZELDATRI1 0x05
#define G_ZELDATRI2 0x06			// G_DL
#define G_ZELDATRI3 0x07

void RDP_GFX_IdentifyuCode(OSTask * pTask)
{
	DBGConsole_Msg(0, "[GReconfiguring RDP to process ucode %d]", g_ROM.ucode);

	switch (g_ROM.ucode)
	{
	case 0:
		g_dwVertexMult = 10;	
		DBGConsole_Msg(0, "Using uCode 0");
		GFXInstruction[G_MTX] = RDP_GFX_Mtx;				g_szRDPInstrName[G_MTX] = "G_MTX";
		GFXInstruction[G_MOVEMEM] = RDP_GFX_MoveMem;		g_szRDPInstrName[G_MOVEMEM] = "G_MOVEMEM";
		GFXInstruction[G_DMATRI] = RDP_GFX_DmaTri;			g_szRDPInstrName[G_DMATRI] = "G_DMATRI";
		GFXInstruction[G_DL] = RDP_GFX_DL;					g_szRDPInstrName[G_DL] = "G_DL";
		GFXInstruction[G_VTX] = RDP_GFX_Vtx_Mario;			g_szRDPInstrName[G_VTX] = "G_VTX";
		GFXInstruction[G_TRI2] = RDP_GFX_Tri2_Goldeneye;	g_szRDPInstrName[G_TRI2] = "G_TRI2";
		GFXInstruction[7] = RDP_GFX_DLInMem;				g_szRDPInstrName[7] = "G_DLINMEM";
		break;
		
	case 1:
		g_dwVertexMult = 2;	
		DBGConsole_Msg(0, "Using uCode 1");
		GFXInstruction[G_MTX] = RDP_GFX_Mtx;				g_szRDPInstrName[G_MTX] = "G_MTX";
		GFXInstruction[G_MOVEMEM] = RDP_GFX_MoveMem;		g_szRDPInstrName[G_MOVEMEM] = "G_MOVEMEM";
		GFXInstruction[G_DMATRI] = RDP_GFX_DmaTri;			g_szRDPInstrName[G_DMATRI] = "G_DMATRI";
		GFXInstruction[G_DL] = RDP_GFX_DL;					g_szRDPInstrName[G_DL] = "G_DL";
		GFXInstruction[G_VTX] = RDP_GFX_Vtx_MarioKart;		g_szRDPInstrName[G_VTX] = "G_VTX";
		GFXInstruction[G_TRI2] = RDP_GFX_Tri2_Mariokart;	g_szRDPInstrName[G_TRI2] = "G_TRI2";
		GFXInstruction[7] = RDP_GFX_DLInMem;				g_szRDPInstrName[7] = "G_DLINMEM";
		break;

	case 4:
		g_dwVertexMult = 5;	
		DBGConsole_Msg(0, "Using uCode 4");
		GFXInstruction[G_MTX] = RDP_GFX_Mtx;				g_szRDPInstrName[G_MTX] = "G_MTX";
		GFXInstruction[G_MOVEMEM] = RDP_GFX_MoveMem;		g_szRDPInstrName[G_MOVEMEM] = "G_MOVEMEM";
		GFXInstruction[G_DMATRI] = RDP_GFX_DmaTri;			g_szRDPInstrName[G_DMATRI] = "G_DMATRI";
		GFXInstruction[G_DL] = RDP_GFX_DL;					g_szRDPInstrName[G_DL] = "G_DL";
		GFXInstruction[G_VTX] = RDP_GFX_Vtx_WaveraceUS;		g_szRDPInstrName[G_VTX] = "G_VTX";
		GFXInstruction[G_TRI2] = RDP_GFX_Tri2_Mariokart;	g_szRDPInstrName[G_TRI2] = "G_TRI2";
		GFXInstruction[7] = RDP_GFX_DLInMem;				g_szRDPInstrName[7] = "G_DLINMEM";
		break;

	case 5:
		g_dwVertexMult = 2;	
		DBGConsole_Msg(0, "Using uCode 5");
		GFXInstruction[G_ZELDAVTX] = RDP_GFX_Vtx_Zelda;		g_szRDPInstrName[G_ZELDAVTX] = "G_ZVTX";
		GFXInstruction[G_ZELDACULLDL] = RDP_GFX_ZeldaCullDL;g_szRDPInstrName[G_ZELDACULLDL] = "G_ZCULLDL";
		GFXInstruction[G_VTX] = RDP_GFX_ZeldaUnk4;			g_szRDPInstrName[G_VTX] = "G_ZUNK4";
		GFXInstruction[G_ZELDATRI1] = RDP_GFX_ZeldaTri1;	g_szRDPInstrName[G_ZELDATRI1] = "G_ZTRI1";
		GFXInstruction[G_ZELDATRI2] = RDP_GFX_ZeldaTri2;	g_szRDPInstrName[G_ZELDATRI2] = "G_ZTRI2";
		GFXInstruction[7] = RDP_GFX_ZeldaTri3;				g_szRDPInstrName[G_ZELDATRI3] = "G_ZTRI3";
		GFXInstruction[G_TRI2] = RDP_GFX_Tri2_Goldeneye;	g_szRDPInstrName[G_TRI2] = "G_TRI2";
		break;

	case 6:
		g_dwVertexMult = 10;	
		DBGConsole_Msg(0, "Using uCode 6");
		GFXInstruction[G_MTX] = RDP_GFX_MtxDKR;				g_szRDPInstrName[G_MTX] = "G_MTX";
		GFXInstruction[G_MOVEMEM] = RDP_GFX_MoveMem;		g_szRDPInstrName[G_MOVEMEM] = "G_MOVEMEM";
		GFXInstruction[G_DMATRI] = RDP_GFX_DmaTri;			g_szRDPInstrName[G_DMATRI] = "G_DMATRI";
		GFXInstruction[G_DL] = RDP_GFX_DL;					g_szRDPInstrName[G_DL] = "G_DL";
		GFXInstruction[G_VTX] = RDP_GFX_Vtx_DKR;			g_szRDPInstrName[G_VTX] = "G_VTX";
		GFXInstruction[G_TRI2] = RDP_GFX_Tri2_Goldeneye;	g_szRDPInstrName[G_TRI2] = "G_TRI2";
		GFXInstruction[7] = RDP_GFX_DLInMem;				g_szRDPInstrName[7] = "G_DLINMEM";
		break;


	default:
		g_dwVertexMult = 10;	
		DBGConsole_Msg(0, "Using default uCode");
		GFXInstruction[G_MTX] = RDP_GFX_Mtx;				g_szRDPInstrName[G_MTX] = "G_MTX";
		GFXInstruction[G_MOVEMEM] = RDP_GFX_MoveMem;		g_szRDPInstrName[G_MOVEMEM] = "G_MOVEMEM";
		GFXInstruction[G_DMATRI] = RDP_GFX_DmaTri;			g_szRDPInstrName[G_DMATRI] = "G_DMATRI";
		GFXInstruction[G_DL] = RDP_GFX_DL;					g_szRDPInstrName[G_DL] = "G_DL";
		GFXInstruction[G_VTX] = RDP_GFX_Vtx_Mario;			g_szRDPInstrName[G_VTX] = "G_VTX";
		GFXInstruction[G_TRI2] = RDP_GFX_Tri2_Goldeneye;	g_szRDPInstrName[G_TRI2] = "G_TRI2";
		GFXInstruction[7] = RDP_GFX_DLInMem;				g_szRDPInstrName[7] = "G_DLINMEM";

	}
}




void RDP_GFX_ExecuteTask(OSTask * pTask)
{
	u32 dwPC;
	u32 dwCmd0;
	u32 dwCmd1;
	HRESULT hr;		
	u32 dwWidth;
	u32 dwHeight;
	DList dl;

	if (g_bDetermineduCode == FALSE)
	{
		RDP_GFX_IdentifyuCode(pTask);
		g_bDetermineduCode = TRUE;
	}

	// Initialise stack
	g_dwPCStack.clear();
	dl.addr = (u32)pTask->t.data_ptr;
	dl.limit = ~0;
	g_dwPCStack.push_back(dl);


	// Check if we need to purge
	if (g_dwRDPTime - g_dwLastPurgeTimeTime > 5000)
	{
		TH_PurgeOldTextures();
		g_dwLastPurgeTimeTime = g_dwRDPTime;
	}

	g_dwNumTexturesIgnored = 0;
	g_dwNumDListsCulled = 0;
	g_dwNumTrisRendered = 0;
	g_dwNumTrisClipped = 0;
	g_dwNumVertices = 0;

	if (g_bDropAllTextures)
	{
		TH_DropTextures();
		g_bDropAllTextures = FALSE;
	}
		

	// Lock the graphics context here.
	if (g_bRDPEnableGfx && g_Renderer != NULL)
	{
			g_Renderer->SetScreenMult( 640.0f / (float)g_dwViWidth, 480.0f / (float)g_dwViHeight );
			g_Renderer->Reset();

            // Frame beginnen
            pvr_wait_ready();
        	pvr_scene_begin();
        	pvr_list_begin( PVR_LIST_OP_POLY );
			g_Renderer->SetViewport(0, 0, g_dwViWidth, g_dwViHeight);

			// Process the entire display list in one go
			while (!g_bRDPHalted)
			{
				// Current PC is the last value on the stack
				dwPC = g_dwPCStack[g_dwPCStack.size() - 1].addr;

				if (dwPC > g_dwRamSize)
				{
					DBGConsole_Msg(0, "Display list PC is out of range: 0x%08x", dwPC);
					break;
				}

				dwCmd0 = g_pu32RamBase[(dwPC>>2)+0];
				dwCmd1 = g_pu32RamBase[(dwPC>>2)+1];

				g_dwPCStack[g_dwPCStack.size() - 1].addr += 8;

				try 
				{
					GFXInstruction[dwCmd0>>24](dwCmd0, dwCmd1);
				}

				catch (...)
				{
					DBGConsole_Msg(0, "Exception processing 0x%08x 0x%08x", dwCmd0, dwCmd1);
					if (!g_bTrapExceptions)
						throw;
					break;
				}

				// Check limit
				if (!g_bRDPHalted)
				{
					g_dwPCStack[g_dwPCStack.size() - 1].limit--;
					if (g_dwPCStack[g_dwPCStack.size() - 1].limit == ~0)
					{
						// If we're here, then we musn't have finished with the display list yet
						// Check if this is the lasy display list in the sequence
						if(g_dwPCStack.size() == 1)
						{
							RDPHalt();
							g_dwPCStack.clear();
						}
						else
						{
							g_dwPCStack.pop_back();
						}
					}	

				}
			}

            // Frame abschließen
            pvr_list_finish();
  	        pvr_list_begin( PVR_LIST_TR_POLY );
  	        g_Renderer->FlushTRTris();
            pvr_list_finish();
			pvr_scene_finish();

            // Frames per Second ausgeben
            printFPS();
		//}*/
	}


	/*ID3DXContext * pD3DX = g_GfxContext.GetD3DX();
	if (pD3DX)
	{
		LPDIRECTDRAWSURFACE7 pSurf = pD3DX->GetBackBuffer(0);
		if (pSurf)
		{
			HRESULT hr;
			// Hack to avoid splattering memory on first DL
			if (Memory_VI_GetRegister(VI_ORIGIN_REG) > 0x280)
			{
				DDSURFACEDESC2 ddsd;
				ZeroMemory( &ddsd, sizeof(DDSURFACEDESC2) );
				ddsd.dwSize = sizeof(ddsd);
				hr = pSurf->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT , NULL);

				if (SUCCEEDED(hr))
				{
					LONG y, x;
					u16 * pDst = &g_pu16RamBase[Memory_VI_GetRegister(VI_ORIGIN_REG)/2];

					for (y = 0; y < g_dwViHeight; y++)
					{
						u8 *pSrc = (u8*)ddsd.lpSurface + ((y * dwHeight) / g_dwViHeight) * ddsd.lPitch;
						for (x = 0; x < g_dwViWidth; x++)
						{
							//*p++ = 0x0001;
							pDst[x^0x2] = *(u16*)(pSrc + ((x * dwWidth) / g_dwViWidth));
						}
						pDst = (u16*)((u8*)pDst + x/2);
						//pSrc = (u16*)((u8*)pSrc + ddsd.lPitch);

					}

					pSurf->Unlock(NULL);
				}
			}

			//pSurf->Release();

		}
	}*/

	// Do this regardless!
	Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_DP);
	AddCPUJob(CPU_CHECK_INTERRUPTS);

	if (g_dwNumTrisRendered + g_dwNumTrisClipped > 0)
	{
		/*outputMessage(MSG_INFO, "%dv %dt (%dclip)",
			g_dwNumVertices, g_dwNumTrisRendered + g_dwNumTrisClipped, g_dwNumTrisClipped);*/
	}
	//outputMessage(MSG_INFO, "%d tex load ignored", g_dwNumTexturesIgnored);
	//outputMessage(MSG_INFO, "%d dlists culled", g_dwNumDListsCulled);

	// Have image displayed
	DPF(DEBUG_MEMORY_PI, "******************* DONE **************");
}



void RDP_GFX_PopDL()
{
	if (g_dwPCStack.size() == 0)
	{
		RDPHalt();			// Stop, damn it!
		return;
	}

	// Check if this is the lasy display list in the sequence
	if(g_dwPCStack.size() == 1)
	{
		RDPHalt();
		g_dwPCStack.clear();
		return;
	}

	// If we're here, then we musn't have finished with the display list yet
	g_dwPCStack.pop_back();

}



void RDP_MirrorEmulator_DrawLine(DrawInfo& destInfo, DrawInfo& srcInfo, LPu32 pSource, LPu32 pDest, u32 nWidth, BOOL bFlipLeftRight)
{
	if(!bFlipLeftRight)
	{
		memcpy(pDest, pSource, nWidth * 4);
	}
	else
	{
		LPu32 pMaxDest = pDest + nWidth;
		pSource += nWidth - 1;
		for(; pDest < pMaxDest; pDest++, pSource--)
		{
			*pDest = *pSource;
		}
	}
}

void RDP_MirrorEmulator_Draw(DrawInfo& destInfo, DrawInfo& srcInfo, u32 nDestX, u32 nDestY, BOOL bFlipLeftRight, BOOL bFlipUpDown)
{
	LPBYTE pDest = (LPBYTE)((u32)destInfo.lpSurface + (destInfo.lPitch * nDestY) + (4 * nDestX));
	LPBYTE pMaxDest = pDest + (destInfo.lPitch * srcInfo.dwHeight);
	LPBYTE pSource = (LPBYTE)(srcInfo.lpSurface);
	if(!bFlipUpDown)
	{
		for(; pDest < pMaxDest; pDest += destInfo.lPitch, pSource += srcInfo.lPitch)
		{
			RDP_MirrorEmulator_DrawLine(destInfo, srcInfo, (LPu32)pSource, (LPu32)pDest, srcInfo.dwWidth, bFlipLeftRight);
		}
	}
	else
	{
		pSource += (srcInfo.lPitch * (srcInfo.dwHeight - 1));
		for(; pDest < pMaxDest; pDest += destInfo.lPitch, pSource -= srcInfo.lPitch)
		{
			RDP_MirrorEmulator_DrawLine(destInfo, srcInfo, (LPu32)pSource, (LPu32)pDest, srcInfo.dwWidth, bFlipLeftRight);
		}
	}
}

SurfaceHandler* RDP_GetSurfaceHandler(u32 dwTile, TextureEntry *pEntry)
{
/*	if((g_Tiles[dwTile].bMirrorS) && (g_Tiles[dwTile].bMirrorT) && !(g_D3DDeviceDesc.dpcTriCaps.dwTextureAddressCaps & D3DPTADDRESSCAPS_MIRROR))
	{
		if(pEntry->pMirroredSurface)
		{
			return pEntry->pMirroredSurface;
		}
		else
		{
			SurfaceHandler* pSurfaceHandler = NULL;

			// FIXME: Compute the correct values. 2/2 seems to always work correctly in Mario64
			u32 nXTimes = 2;
			u32 nYTimes = 2;
			
			DrawInfo srcInfo;		
			pEntry->pSurface->StartUpdate(&srcInfo);
			u32 nWidth = srcInfo.dwWidth;
			u32 nHeight = srcInfo.dwHeight;

			pSurfaceHandler = new SurfaceHandler(nWidth * nXTimes, nHeight * nYTimes);
			DrawInfo destInfo;
			pSurfaceHandler->StartUpdate(&destInfo);
			
			
			for(u32 nY = 0; nY < nYTimes; nY++)
			{
				for(u32 nX = 0; nX < nXTimes; nX++)
				{
					RDP_MirrorEmulator_Draw(destInfo, srcInfo, nWidth * nX, nHeight * nY, nX & 0x1, nY & 0x1);
				}
			}

			pSurfaceHandler->EndUpdate(&destInfo);
			pEntry->pSurface->EndUpdate(&srcInfo);

			pEntry->pMirroredSurface = pSurfaceHandler;

			return pSurfaceHandler;
		}
	}*/
	return NULL;
}

void RDP_EnableTexturing(u32 dwTile)
{
	u32 dwTileWidth;
	u32 dwTileHeight;
	u32 dwTLutFmt;
	u32 dwPitch;
	u32 dwPalOffset;
	TextureEntry *pEntry = NULL;

	dwTLutFmt = g_dwOtherModeH& (0x3<<G_MDSFT_TEXTLUT);

	// Now initialise surfaces
	dwTileWidth = (g_Tiles[dwTile].nLRS - g_Tiles[dwTile].nULS);
	dwTileHeight= (g_Tiles[dwTile].nLRT - g_Tiles[dwTile].nULT);

	if (dwTileWidth == 0 || dwTileHeight == 0)
	{
		//DBGConsole_Msg(0, "Ignoring zero-sized tile load");
		return;
	}



	dwPalOffset = 0;

	// Only needs doing for CI, but never mind
	switch (g_Tiles[dwTile].dwSize)
	{
	case G_IM_SIZ_4b: dwPalOffset = 16  * 2 * g_Tiles[dwTile].dwPalette; break;
	case G_IM_SIZ_8b: dwPalOffset = 256 * 2 * g_Tiles[dwTile].dwPalette; break;
	}	

	
	if (g_nLastLoad == LOAD_BLOCK)
	{
		// HACK: In Mario, the texture for the Head bit is offset for some reason
		g_Tiles[dwTile].nLRS -= g_Tiles[dwTile].nULS;
		g_Tiles[dwTile].nLRT -= g_Tiles[dwTile].nULT;			
		g_Tiles[dwTile].nULS = 0;
		g_Tiles[dwTile].nULT = 0;			

		// It was a block load - the pitch is determined by the tile size
		dwPitch = g_Tiles[dwTile].dwPitch;
	}
	else
	{
		// It was a tile - the pitch is set when the tile is loaded
		g_bDXTZero = 0;
		dwPitch = g_dwLoadTilePitch;
	}

	GetTextureInfo gti;


	gti.dwAddress = g_dwLoadAddress;
	gti.dwFormat = g_Tiles[dwTile].dwFormat;
	gti.dwSize = g_Tiles[dwTile].dwSize;
	gti.nLeft = g_Tiles[dwTile].nULS;
	gti.nTop = g_Tiles[dwTile].nULT;
	gti.dwWidth = dwTileWidth;
	gti.dwHeight = dwTileHeight;
	gti.dwPitch = dwPitch;
	gti.dwPalAddress = g_dwPalAddress + dwPalOffset;
	gti.dwPalSize = g_dwPalSize;
	gti.dwTLutFmt = dwTLutFmt;
	gti.bSwapped = g_bDXTZero;

	// Hack - Extreme-G specifies RGBA/8 textures, but they're really CI8
	if (gti.dwFormat == G_IM_FMT_RGBA &&
		(gti.dwSize == G_IM_SIZ_8b || gti.dwSize == G_IM_SIZ_4b))
		gti.dwFormat = G_IM_FMT_CI;

	if (gti.dwFormat == G_IM_FMT_CI)
	{
		if (gti.dwTLutFmt == G_TT_NONE)
			gti.dwTLutFmt = G_TT_RGBA16;		// Force RGBA

	}

	pEntry = TH_GetTexture(&gti);
	if (pEntry != NULL && pEntry->pSurface != NULL)
	{	
		//Lkb mirror support
		SurfaceHandler* pSurfaceHandler = NULL;
		
		pSurfaceHandler = RDP_GetSurfaceHandler(dwTile, pEntry);	

		g_Renderer->SetTexture( (pSurfaceHandler != NULL)? pSurfaceHandler : pEntry->pSurface,
								pEntry->nLeft, 
								pEntry->nTop,
								pEntry->dwWidth,
								pEntry->dwHeight);

		//delete pSurfaceHandler;

		// Initialise the clamping state...what if clamp AND mirror?
		if (g_Tiles[dwTile].bMirrorS)		g_Renderer->SetAddressU( 0, D3DTADDRESS_MIRROR );
		else if (g_Tiles[dwTile].bClampS)	g_Renderer->SetAddressU( 0, D3DTADDRESS_CLAMP );
		else								g_Renderer->SetAddressU( 0, D3DTADDRESS_WRAP );
		
		if (g_Tiles[dwTile].bMirrorT)		g_Renderer->SetAddressV( 0, D3DTADDRESS_MIRROR );
		else if (g_Tiles[dwTile].bClampT)	g_Renderer->SetAddressV( 0, D3DTADDRESS_CLAMP );
		else								g_Renderer->SetAddressV( 0, D3DTADDRESS_WRAP );
			
	}
	else
	{
		g_Renderer->SetTexture( NULL, 0, 0, 64, 64 );
	}
}





void RDP_MoveMemLight(u32 dwLight, u32 dwAddress)
{
	s8 * pcBase = g_ps8RamBase + dwAddress;
	u32 * pdwBase = (u32 *)pcBase;

	g_N64Lights[dwLight].dwRGBA     = pdwBase[0];
	g_N64Lights[dwLight].dwRGBACopy = pdwBase[1];
	g_N64Lights[dwLight].x			= (float)pcBase[8 ^ 0x3];
	g_N64Lights[dwLight].y			= (float)pcBase[9 ^ 0x3];
	g_N64Lights[dwLight].z			= (float)pcBase[10 ^ 0x3];
					
	if (dwLight == g_dwAmbientLight)
	{
		u32 dwCol = RGBA_MAKE( (g_N64Lights[dwLight].dwRGBA >> 24)&0xFF,
					  (g_N64Lights[dwLight].dwRGBA >> 16)&0xFF,
					  (g_N64Lights[dwLight].dwRGBA >>  8)&0xFF, 0XFF);

		g_Renderer->SetAmbientLight( dwCol );
	}
	else
	{
		g_Renderer->SetLightCol(dwLight, g_N64Lights[dwLight].dwRGBA);
		if (pdwBase[2] != 0)	// Direction is 0!
		{
			g_Renderer->SetLightDirection(dwLight, 
										  g_N64Lights[dwLight].x,
										  g_N64Lights[dwLight].y,
										  g_N64Lights[dwLight].z);
		}
	}
}

void RDP_MoveMemViewport(u32 dwAddress)
{
	s16 scale[4];
	s16 trans[4];

	// dwAddress is offset into RD_RAM of 8 x 16bits of data...
	scale[0] = *(s16 *)(g_pu8RamBase + ((dwAddress+(0*2))^0x2));
	scale[1] = *(s16 *)(g_pu8RamBase + ((dwAddress+(1*2))^0x2));
	scale[2] = *(s16 *)(g_pu8RamBase + ((dwAddress+(2*2))^0x2));
	scale[3] = *(s16 *)(g_pu8RamBase + ((dwAddress+(3*2))^0x2));

	trans[0] = *(s16 *)(g_pu8RamBase + ((dwAddress+(4*2))^0x2));
	trans[1] = *(s16 *)(g_pu8RamBase + ((dwAddress+(5*2))^0x2));
	trans[2] = *(s16 *)(g_pu8RamBase + ((dwAddress+(6*2))^0x2));
	trans[3] = *(s16 *)(g_pu8RamBase + ((dwAddress+(7*2))^0x2));


	LONG nCenterX = trans[0]/4;
	LONG nCenterY = trans[1]/4;
	LONG nWidth   = scale[0]/4;
	LONG nHeight  = scale[1]/4;


	LONG nLeft = nCenterX - nWidth;
	LONG nTop  = nCenterY - nHeight;
	LONG nRight= nCenterX + nWidth;
	LONG nBottom= nCenterY + nHeight;

	// With D3D we had to ensure that the vp coords are positive, so
	// we truncated them to 0. This happens a lot, as things
	// seem to specify the scale as the screen w/2 h/2

	g_Renderer->SetViewport(nLeft, nTop, nRight, nBottom);
}

#define RDP_NOIMPL_WARN(op) { DBGConsole_Msg(0, op); }
//#define RDP_NOIMPL_WARN 1 ? (void)0 : (void)
inline void RDP_NOIMPL(LPCTSTR op, u32 dwCmd0, u32 dwCmd1)
{
	/*RDPHalt();*/
}

//Nintro64 uses Sprite2d 
void RDP_GFX_Nothing(u32 dwCmd0, u32 dwCmd1)
{
	// Terminate!
	{
		DBGConsole_Msg(0, "Warning, DL cut short with unknown command: 0x%08x 0x%08x",
			dwCmd0, dwCmd1);
		RDPHalt();
		g_dwPCStack.clear();
	}

}
void RDP_GFX_AeroUnk1(u32 dwCmd0, u32 dwCmd1)
{
}

void RDP_GFX_Zelda2Unk1(u32 dwCmd0, u32 dwCmd1)
{
}
void RDP_GFX_Zelda2Unk2(u32 dwCmd0, u32 dwCmd1)
{
}

void RDP_GFX_ZeldaUnk1(u32 dwCmd0, u32 dwCmd1)
{
}
void RDP_GFX_ZeldaUnk2(u32 dwCmd0, u32 dwCmd1)
{
}
void RDP_GFX_ZeldaUnk3(u32 dwCmd0, u32 dwCmd1)
{
}
void RDP_GFX_ZeldaUnk4(u32 dwCmd0, u32 dwCmd1)
{
}


void RDP_GFX_SPNOOP(u32 dwCmd0, u32 dwCmd1)		{  }


void RDP_GFX_ZeldaMoveWordTri(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwOffset = (dwCmd0 >> 16) & 0xFF;
	u32 dwVert   = ((dwCmd0      ) & 0xFFFF) / 2;
	u32 dwValue  = dwCmd1;

	// Data for other commands?
	switch (dwOffset)
	{
	case 0x00:		// Pos x/y
	case 0x04:		// Pos z/flag
	case 0x08:		// Texture coords
	case 0x0c:		// Color/Alpha/Normal
		break;
	case 0x14:		// Texture
		{
			short tu = (dwValue>>16);
			short tv = (dwValue & 0xFFFF);
			g_Renderer->SetVtxTextureCoord(dwVert, (short)tu, (short)tv);
		}
		break;
	default:
		break;
	}
}

// YoshiStory does this - 0xc2
void RDP_GFX_YS_Unk1(u32 dwCmd0, u32 dwCmd1)
{
	// 
	static BOOL bWarned = FALSE;
	if (!bWarned)
	{
		RDP_NOIMPL("RDP: RDP_GFX_YS_Unk1 (0x%08x 0x%08x)", dwCmd0, dwCmd1);
		bWarned = TRUE;
	}

}
// AllStarTennis does this - 0xaf...
void RDP_GFX_AST_Unk1(u32 dwCmd0, u32 dwCmd1)
{
	// 
	static BOOL bWarned = FALSE;
	if (!bWarned)
	{
		RDP_NOIMPL("RDP: RDP_GFX_AST_Unk1 (0x%08x 0x%08x)", dwCmd0, dwCmd1);
		bWarned = TRUE;
	}

}



void RDP_GFX_Reserved3(u32 dwCmd0, u32 dwCmd1)
{		
	// Spiderman
	static BOOL bWarned = FALSE;

	if (!bWarned)
	{
		RDP_NOIMPL("RDP: Reserved3 (0x%08x 0x%08x)", dwCmd0, dwCmd1);
		bWarned = TRUE;
	}
}

void RDP_GFX_Sprite2D(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwAddress = RDPSegAddr(dwCmd1);
}



void RDP_GFX_NOOP(u32 dwCmd0, u32 dwCmd1)
{
}


void RDP_GFX_SetKeyGB(u32 dwCmd0, u32 dwCmd1)
{
	static BOOL bWarned = FALSE;
	if (!bWarned)
	{
		RDP_NOIMPL("RDP: SetKeyGB 0x%08x 0x%08x", dwCmd0, dwCmd1);
		bWarned = TRUE;
	}

}
void RDP_GFX_SetKeyR(u32 dwCmd0, u32 dwCmd1)
{
	static BOOL bWarned = FALSE;
	if (!bWarned)
	{
		RDP_NOIMPL("RDP: SetKeyR 0x%08x 0x%08x", dwCmd0, dwCmd1);
		bWarned = TRUE;
	}
}

void RDP_GFX_SetConvert(u32 dwCmd0, u32 dwCmd1)
{
	
	// Spiderman
	static BOOL bWarned = FALSE;
	if (!bWarned)
	{
		RDP_NOIMPL("RDP: SetConvert 0x%08x 0x%08x", dwCmd0, dwCmd1);
		bWarned = TRUE;
	}


}
void RDP_GFX_SetPrimDepth(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwZ  = (dwCmd1 >> 16) & 0xFFFF;
	u32 dwDZ = (dwCmd1      ) & 0xFFFF;
	
	// Not implemented!
}

void RDP_GFX_RDPSetOtherMode(u32 dwCmd0, u32 dwCmd1)
{
}



void RDP_GFX_LoadTLut(u32 dwCmd0, u32 dwCmd1)
{
	static const char *textluttype[4] = {"None", "?", "RGBA16", "IA16"};
	u32 dwOffset;
	u32 dwULS   = ((dwCmd0 >> 12) & 0xfff)/4;
	u32 dwULT   = ((dwCmd0      ) & 0xfff)/4;
	u32 dwTile  = ((dwCmd1 >> 24) & 0x07);
	u32 dwLRS   = ((dwCmd1 >> 12) & 0xfff)/4;
	u32 dwLRT   = ((dwCmd1      ) & 0xfff)/4;

	u32 dwTLutFmt = (g_dwOtherModeH >> G_MDSFT_TEXTLUT)&0x3;
	u32 dwCount = (dwLRS - dwULS);

	// Format is always 16bpp - RGBA16 or IA16:
	// I've no idea why these two are added - seems to work for 007!
	dwOffset = (dwULS + dwULT)*2;

	g_dwPalAddress = g_TI.dwAddr + dwOffset;
	g_dwPalSize = dwCount;
}



void RDP_GFX_RDPHalf_Cont(u32 dwCmd0, u32 dwCmd1)	{ /*DL_PF("RDPHalf_Cont: (Ignored)");*/ }
void RDP_GFX_RDPHalf_2(u32 dwCmd0, u32 dwCmd1)		{ /*DL_PF("RDPHalf_2: (Ignored)");*/ }
void RDP_GFX_RDPHalf_1(u32 dwCmd0, u32 dwCmd1)		{ /*DL_PF("RDPHalf_1: (Ignored)");*/ }
void RDP_GFX_RDPLoadSync(u32 dwCmd0, u32 dwCmd1)	{ /*DL_PF("LoadSync: (Ignored)");*/ }
void RDP_GFX_RDPPipeSync(u32 dwCmd0, u32 dwCmd1)	{ /*DL_PF("PipeSync: (Ignored)");*/ }
void RDP_GFX_RDPTileSync(u32 dwCmd0, u32 dwCmd1)	{ /*DL_PF("TileSync: (Ignored)");*/ }

void RDP_GFX_RDPFullSync(u32 dwCmd0, u32 dwCmd1)
{ 
	// We now do this regardless
	Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_DP);
	AddCPUJob(CPU_CHECK_INTERRUPTS);
	
	/*DL_PF("FullSync: (Generating Interrupt)");*/
}

void RDP_GFX_DL(u32 dwCmd0, u32 dwCmd1)
{	
	DList dl;
	u32 dwPush = (dwCmd0 >> 16) & 0xFF;
	u32 dwAddr = RDPSegAddr(dwCmd1);
	
	switch (dwPush)
	{
	case G_DL_PUSH:
		dl.addr = dwAddr;
		dl.limit = ~0;
		g_dwPCStack.push_back( dl );

		break;
	case G_DL_NOPUSH:
		g_dwPCStack[g_dwPCStack.size() - 1].addr = dwAddr;
		g_dwPCStack[g_dwPCStack.size() - 1].limit = ~0;
		break;
	}
}

// BB2k
// DKR
//00229B70: 07020010 000DEFC8 CMD G_DLINMEM  Displaylist at 800DEFC8 (stackp 1, limit 2)
//00229A58: 06000000 800DE520 CMD G_DL  Displaylist at 800DE520 (stackp 1, limit 0)
//00229B90: 07070038 00225850 CMD G_DLINMEM  Displaylist at 80225850 (stackp 1, limit 7)

/* flags to inhibit pushing of the display list (on branch) */
//#define G_DL_PUSH		0x00
//#define G_DL_NOPUSH		0x01


void RDP_GFX_DLInMem(u32 dwCmd0, u32 dwCmd1)
{
	DList dl;
	//DBGConsole_Msg(0, "DLInMem: 0x%08x 0x%08x", dwCmd0, dwCmd1);

	u32 dwLimit = (dwCmd0 >> 16) & 0xFF;
	u32 dwPush = G_DL_PUSH; //(dwCmd0 >> 16) & 0xFF;
	u32 dwAddr = 0x00000000 | dwCmd1; //RDPSegAddr(dwCmd1);
	
	switch (dwPush)
	{
	case G_DL_PUSH:
		dl.addr = dwAddr;
		dl.limit = dwLimit;
		g_dwPCStack.push_back( dl );

		break;
	case G_DL_NOPUSH:
		g_dwPCStack[g_dwPCStack.size() - 1].addr = dwAddr;
		g_dwPCStack[g_dwPCStack.size() - 1].limit = dwLimit;
		break;
	}
}



void RDP_GFX_ZeldaDL(u32 dwCmd0, u32 dwCmd1)
{
	DList dl;

	u32 dwPush = (dwCmd0 >> 16) & 0xFF;
	u32 dwAddr = RDPSegAddr(dwCmd1);
	
	switch (dwPush)
	{
	case G_DL_PUSH:
		dl.addr = dwAddr;
		dl.limit = ~0;
		g_dwPCStack.push_back( dl );

		break;
	case G_DL_NOPUSH:
		g_dwPCStack[g_dwPCStack.size() - 1].addr = dwAddr;
		g_dwPCStack[g_dwPCStack.size() - 1].limit = ~0;
		break;
	}
}



void RDP_GFX_EndDL(u32 dwCmd0, u32 dwCmd1)
{
	RDP_GFX_PopDL();
}

void RDP_GFX_ZeldaEndDL(u32 dwCmd0, u32 dwCmd1)
{
	RDP_GFX_PopDL();
}


void RDP_GFX_CullDL(u32 dwCmd0, u32 dwCmd1)
{
}

void RDP_GFX_ZeldaCullDL(u32 dwCmd0, u32 dwCmd1)
{
}


void RDP_GFX_MoveWord(u32 dwCmd0, u32 dwCmd1)
{
	// Type of movement is in low 8bits of cmd0.

	u32 dwIndex = dwCmd0 & 0xFF;
	u32 dwOffset = (dwCmd0 >> 8) & 0xFFFF;


	switch (dwIndex)
	{
	case G_MW_MATRIX:
		RDP_NOIMPL_WARN("G_MW_MATRIX Not Implemented");

		break;
	case G_MW_NUMLIGHT:
		//#define NUML(n)		(((n)+1)*32 + 0x80000000)
		if (g_ROM.ucode != 6)
		{
			u32 dwNumLights = ((dwCmd1-0x80000000)/32) - 1;

			g_dwAmbientLight = dwNumLights;
			g_Renderer->SetNumLights(dwNumLights);
		}
		else
		{
			// DKR
			u32 dwNumLights = dwCmd1;

			g_dwAmbientLight = dwNumLights;
			g_Renderer->SetNumLights(dwNumLights);

		}
		break;
	case G_MW_CLIP:
		break;
	case G_MW_SEGMENT:
		{
			u32 dwSegment = (dwOffset >> 2) & 0xF;
			u32 dwBase = dwCmd1&0x00FFFFFF;
			g_dwSegment[dwSegment] = dwBase;
		}
		break;
	case G_MW_FOG:
		{
			WORD wMult = (WORD)((dwCmd1 >> 16) & 0xFFFF);
			WORD wOff  = (WORD)((dwCmd1      ) & 0xFFFF);

			float fMult = (float)(short)wMult;
			float fOff = (float)(short)wOff;

			float fMin = 1.0f - (fOff/ fMult);
			float fMax = (256.0f/ fMult) + fMin;

			//	DBGConsole_Msg(0, "MoveWord: G_MW_FOG. Mult = 0x%04x, Off = 0x%04x", wMult, wOff);
			//	DBGConsole_Msg(0, "   => Min:%f, Max:%f", fMin/1000.0f, fMax/1000.0f);
			//g_Renderer->SetFogMultOff(fMult / 0x7fff , fOff / 0x7fff);
			//g_Renderer->SetFogMultOff((float)wMult / 0x7fff , (float)wOff / 0x7fff);

			// Scale min/max from 0..1000 to -1 to 1
			fMin -= 1.0f;
			fMax -= 1.0f;

			g_Renderer->SetFogMultOff(1.0f / (fMax - fMin), -fMin / (fMax - fMin));
		//	DBGConsole_Msg(0, "Fog Mult = 0x%04x, Off = 0x%04x => Min:%f, Max:%f", wMult, wOff, fMin, fMax);
		//	DBGConsole_Msg(0, "Fog Mult = %f Off = %f", 1.0f / (fMax - fMin), -fMin / (fMax - fMin));


		}
		//RDP_NOIMPL_WARN("G_MW_FOG Not Implemented");
		break;
	case G_MW_LIGHTCOL:
		if (g_ROM.ucode != 6)
		{
			u32 dwLight = dwOffset / 0x20;
			u32 dwField = (dwOffset & 0x7);

			switch (dwField)
			{
			case 0:
				//g_N64Lights[dwLight].dwRGBA = dwCmd1;
				// Light col, not the copy
				if (dwLight == g_dwAmbientLight)
				{
					g_Renderer->SetAmbientLight( (dwCmd1>>8) );
				}
				else
				{
					g_Renderer->SetLightCol(dwLight, dwCmd1);
				}
				break;

			case 4:
				break;

			default:
				//DBGConsole_Msg(0, "G_MW_LIGHTCOL with unknown offset 0x%08x", dwField);
				break;
			}
		}
		else
		{
			//DKR
			g_Renderer->ResetMatrices();
		}

		break;
	case G_MW_POINTS:
		RDP_NOIMPL_WARN("G_MW_POINTS Not Implemented");
		break;
	case G_MW_PERSPNORM:
		//RDP_NOIMPL_WARN("G_MW_PESPNORM Not Implemented");		// Used in Starfox - sets to 0xa
	//	if ((short)dwCmd1 != 10)
	//		DBGConsole_Msg(0, "PerspNorm: 0x%04x", (short)dwCmd1);	
		break;
	default:
		RDP_NOIMPL_WARN("Unknown MoveWord");
		break;
	}

}


//0016A710: DB020000 00000018 CMD Zelda_MOVEWORD  Mem[2][00]=00000018 Lightnum=0
//001889F0: DB020000 00000030 CMD Zelda_MOVEWORD  Mem[2][00]=00000030 Lightnum=2


void RDP_GFX_ZeldaMoveWord(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwType   = (dwCmd0 >> 16) & 0xFF;
	u32 dwOffset = (dwCmd0      ) & 0xFFFF;

	switch (dwType)
	{
	case G_MW_NUMLIGHT:
		{
			// Lightnum
			// dwCmd1:
			// 0x18 = 24 = 0 lights
			// 0x30 = 48 = 2 lights

			u32 dwNumLights = (dwCmd1/12) - 2;

			g_dwAmbientLight = dwNumLights;
			g_Renderer->SetNumLights(dwNumLights);
		}
		break;

	case G_MW_CLIP:
		break;

	case G_MW_SEGMENT:
		{
			u32 dwSeg     = dwOffset / 4;
			u32 dwAddress = dwCmd1 & 0x00FFFFFF;			// Hack - convert to physical

			g_dwSegment[dwSeg] = dwAddress;

		}
		break;
	case G_MW_FOG:
		{
			//0x7d00
			//0x8400
			WORD wMult = (WORD)((dwCmd1 >> 16) & 0xFFFF);
			WORD wOff  = (WORD)((dwCmd1      ) & 0xFFFF);

			float fMult = (float)(short)wMult;
			float fOff = (float)(short)wOff;

			float fMin = 1.0f - (fOff/ fMult);
			float fMax = (256.0f / fMult) + fMin;

			//Fog Mult = 0x7d00, Off = 0x8400 => Min:0.996000, Max:1.000000
			//Fog Mult = 0.488281 Off = 0.515625
			//g_Renderer->SetFogMultOff(fMult / 0x7fff , fOff / 0x7fff);
			//g_Renderer->SetFogMultOff((float)wMult / 0x10000 , (float)wOff / 0x10000);

			// Scale min/max from 0..2 to -1 to 1
			fMin -= 1.0f;
			fMax -= 1.0f;

			g_Renderer->SetFogMultOff(1.0f / (fMax - fMin), -fMin / (fMax - fMin));
		//	DBGConsole_Msg(0, "Fog Mult = 0x%04x, Off = 0x%04x => Min:%f, Max:%f", wMult, wOff, fMin, fMax);
		//	DBGConsole_Msg(0, "Fog Mult = %f Off = %f", 1.0f / (fMax - fMin), -fMin / (fMax - fMin));

		}
		break;
	case G_MW_LIGHTCOL:
		{
			//ZeldaMoveWord: 0xdb0a0000 0xafafaf00
			//ZeldaMoveWord: 0xdb0a0004 0xafafaf00
			//ZeldaMoveWord: 0xdb0a0018 0x46464600
			//ZeldaMoveWord: 0xdb0a001c 0x46464600

			u32 dwLight = dwOffset / 0x18;
			u32 dwField = (dwOffset & 0x7);

			switch (dwField)
			{
			case 0:
				//g_N64Lights[dwLight].dwRGBA = dwCmd1;
				// Light col, not the copy
				if (dwLight == g_dwAmbientLight)
				{
					g_Renderer->SetAmbientLight( (dwCmd1>>8) );
				}
				else
				{
					g_Renderer->SetLightCol(dwLight, dwCmd1);
				}
				break;

			case 4:
				break;

			default:
				//DBGConsole_Msg(0, "G_MW_LIGHTCOL with unknown offset 0x%08x", dwField);
				break;
			}


		}
		break;

	case G_MW_PERSPNORM:
		//if ((short)dwCmd1 != 10)
		//	DBGConsole_Msg(0, "PerspNorm: 0x%04x", (short)dwCmd1);	
		break;

	default:
		{

		}
		break;
	}
}

void RDP_GFX_MoveMem(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwType    = (dwCmd0>>16)&0xFF;
	u32 dwLength  = (dwCmd0)&0xFFFF;
	u32 dwAddress = RDPSegAddr(dwCmd1);

	switch (dwType) {
		case G_MV_VIEWPORT:
			{
				RDP_MoveMemViewport(dwAddress);
			}
			break;
		case G_MV_LOOKATY:
			//RDP_NOIMPL_WARN("G_MV_LOOKATY Not Implemented");
			break;
		case G_MV_LOOKATX:
			//RDP_NOIMPL_WARN("G_MV_LOOKATX Not Implemented");
			break;
		case G_MV_L0:
		case G_MV_L1:
		case G_MV_L2:
		case G_MV_L3:
		case G_MV_L4:
		case G_MV_L5:
		case G_MV_L6:
		case G_MV_L7:
			{
				u32 dwLight = (dwType-G_MV_L0)/2;

				// Ensure dwType == G_MV_L0 -- G_MV_L7
				//	if (dwType < G_MV_L0 || dwType > G_MV_L7 || ((dwType & 0x1) != 0))
				//		break;


				RDP_MoveMemLight(dwLight, dwAddress);
			}
			break;
		case G_MV_TXTATT:
			RDP_NOIMPL_WARN("G_MV_TXTATT Not Implemented");
			break;
		case G_MV_MATRIX_1:
			RDP_NOIMPL_WARN("G_MV_MATRIX_1 Not Implemented");
			break;
		case G_MV_MATRIX_2:
			RDP_NOIMPL_WARN("G_MV_MATRIX_2 Not Implemented");
			break;
		case G_MV_MATRIX_3:
			RDP_NOIMPL_WARN("G_MV_MATRIX_3 Not Implemented");
			break;
		case G_MV_MATRIX_4:
			RDP_NOIMPL_WARN("G_MV_MATRIX_4 Not Implemented");
			break;
		default:
			RDP_NOIMPL_WARN("Unknown Move Type");
			break;
	}
}
/*

001889F8: DC08060A 80188708 CMD Zelda_MOVEMEM  Movemem[0806] <- 80188708
!light 0 color 0.12 0.16 0.35 dir 0.01 0.00 0.00 0.00 (2 lights) [ 1E285A00 1E285A00 01000000 00000000 ]
data(00188708): 1E285A00 1E285A00 01000000 00000000 
00188A00: DC08090A 80188718 CMD Zelda_MOVEMEM  Movemem[0809] <- 80188718
!light 1 color 0.23 0.25 0.30 dir 0.01 0.00 0.00 0.00 (2 lights) [ 3C404E00 3C404E00 01000000 00000000 ]
data(00188718): 3C404E00 3C404E00 01000000 00000000 
00188A08: DC080C0A 80188700 CMD Zelda_MOVEMEM  Movemem[080C] <- 80188700
!light 2 color 0.17 0.16 0.26 dir 0.23 0.31 0.70 0.00 (2 lights) [ 2C294300 2C294300 1E285A00 1E285A00 ]
*/
/*
ZeldaMoveMem: 0xdc080008 0x801984d8
SetScissor: x0=416 y0=72 x1=563 y1=312 mode=0
// Mtx
ZeldaMoveWord:0xdb0e0000 0x00000041 Ignored
ZeldaMoveMem: 0xdc08000a 0x80198538
ZeldaMoveMem: 0xdc08030a 0x80198548

ZeldeMoveMem: Unknown Type. 0xdc08000a 0x80198518
ZeldeMoveMem: Unknown Type. 0xdc08030a 0x80198528
ZeldeMoveMem: Unknown Type. 0xdc08000a 0x80198538
ZeldeMoveMem: Unknown Type. 0xdc08030a 0x80198548
ZeldeMoveMem: Unknown Type. 0xdc08000a 0x80198518
ZeldeMoveMem: Unknown Type. 0xdc08030a 0x80198528
ZeldeMoveMem: Unknown Type. 0xdc08000a 0x80198538
ZeldeMoveMem: Unknown Type. 0xdc08030a 0x80198548


0xa4001120: <0x0c000487> JAL       0x121c        Seg2Addr(t8)				dram
0xa4001124: <0x332100fe> ANDI      at = t9 & 0x00fe
0xa4001128: <0x937309c1> LBU       s3 <- 0x09c1(k1)							len
0xa400112c: <0x943402f0> LHU       s4 <- 0x02f0(at)							dmem
0xa4001130: <0x00191142> SRL       v0 = t9 >> 0x0005
0xa4001134: <0x959f0336> LHU       ra <- 0x0336(t4)
0xa4001138: <0x080007f6> J         0x1fd8        SpMemXfer
0xa400113c: <0x0282a020> ADD       s4 = s4 + v0								dmem

ZeldaMoveMem: 0xdc08000a 0x8010e830 Type: 0a Len: 08 Off: 4000
ZeldaMoveMem: 0xdc08030a 0x8010e840 Type: 0a Len: 08 Off: 4018
// Light
ZeldaMoveMem: 0xdc08060a 0x800ff368 Type: 0a Len: 08 Off: 4030
ZeldaMoveMem: 0xdc08090a 0x800ff360 Type: 0a Len: 08 Off: 4048
//VP
ZeldaMoveMem: 0xdc080008 0x8010e3c0 Type: 08 Len: 08 Off: 4000

*/
void RDP_GFX_ZeldaMoveMem(u32 dwCmd0, u32 dwCmd1)
{

	u32 dwAddress = RDPSegAddr(dwCmd1);
	u32 dwOffset = (dwCmd0 >> 8) & 0xFFFF;
	u32 dwType    = (dwCmd0     ) & 0xFE;

	u32 dwLen = (dwCmd0 >> 16) & 0xFF;
	u32 dwOffset2 = (dwCmd0 >> 5) & 0x3FFF;

	BOOL bHandled = FALSE;

	switch (dwType)
	{
	case 0x08:
		{
			RDP_MoveMemViewport(dwAddress);
			bHandled = TRUE;
		}
		break;
	case 0x0a:
		{
		switch (dwOffset2)
		{

			/*
					{	{{ {{0,0,0},0,{0,0,0},0,{rightx,righty,rightz},0}}, \
			{ {{0,0x80,0},0,{0,0x80,0},0,{upx,upy,upz},0}}}   }
*/
		case 0x00:
			{
				s8 * pcBase = g_ps8RamBase + dwAddress;
			}
			break;
		case 0x18:
			{
				s8 * pcBase = g_ps8RamBase + dwAddress;
			}
			break;
		default:		//0x30/48/60
			{
				u32 dwLight = (dwOffset2 - 0x30)/0x18;
			}
			break;
		}
		break;

		}
	}

/*
0x00112440: dc08000a 8010e830 G_ZMOVEMEM
    Type: 0a Len: 08 Off: 4000
    00000000 00000000 5a00a600 00000000*/
/*
0x000ff418: dc08060a 800ff368 G_ZMOVEMEM
    Type: 0a Len: 08 Off: 4030
    Light 0:
       RGBA: 0x69696900, RGBACopy: 0x69696900, x: 0.000000, y: -127.000000, z: 0.000000
      (Ambient Light)
0x000ff420: dc08090a 800ff360 G_ZMOVEMEM
    Type: 0a Len: 08 Off: 4048
    Light 1:
       RGBA: 0x46464600, RGBACopy: 0x46464600, x: 105.000000, y: 105.000000, z: 105.000000
      (Normal Light)
*/
	if (!bHandled)
	{
		//DBGConsole_Msg(0, "ZeldeMoveMem: Unknown Type. 0x%08x 0x%08x", dwCmd0, dwCmd1);
		s8 * pcBase = g_ps8RamBase + dwAddress;
		u32 * pdwBase = (u32 *)pcBase;
		LONG i;
		
		for (i = 0; i < 4; i++)
		{
			pdwBase+=4;
		}
	}

}




void RDP_GFX_Mtx(u32 dwCmd0, u32 dwCmd1)
{	
	u32 dwAddress = RDPSegAddr(dwCmd1);
	u32 dwCommand = (dwCmd0>>16)&0xFF;
	u32 dwLength  = (dwCmd0)    &0xFFFF;

	if (dwAddress + 64 > g_dwRamSize)
	{
		DBGConsole_Msg(0, "Mtx: Address invalid (0x%08x)", dwAddress);
		return;
	}

	// Load matrix from dwAddress
	tMatrix mat;
	u32 dwI;
	u32 dwJ;
	const float fRecip = 1.0f / 65536.0f;
	
	for (dwI = 0; dwI < 4; dwI++) {
		for (dwJ = 0; dwJ < 4; dwJ++) {

			SHORT nDataHi = *(SHORT *)(g_pu8RamBase + ((dwAddress+(dwI<<3)+(dwJ<<1)     )^0x2));
			WORD  nDataLo = *(WORD  *)(g_pu8RamBase + ((dwAddress+(dwI<<3)+(dwJ<<1) + 32)^0x2));
			mat.m[dwI][dwJ] = (float)(((LONG)nDataHi<<16) | (nDataLo))*fRecip;
		}
	}
	LONG nLoadCommand = dwCommand & G_MTX_LOAD ? D3DRENDER_LOAD : D3DRENDER_MUL;
	BOOL bPush = dwCommand & G_MTX_PUSH ? TRUE : FALSE;


	if (dwCommand & G_MTX_PROJECTION)
	{
		// So far only Extreme-G seems to Push/Pop projection matrices	
		g_Renderer->SetProjection(mat, bPush, nLoadCommand);
	}
	else
	{
		g_Renderer->SetWorldView(mat, bPush, nLoadCommand);
	}
}



/*
#define	G_MTX_MODELVIEW		0x00
#define	G_MTX_PROJECTION	0x01
#define	G_MTX_MUL			0x00
#define	G_MTX_LOAD			0x02
#define G_MTX_NOPUSH		0x00
#define G_MTX_PUSH			0x04
*/
/*
00229C28: 01400040 002327C0 CMD G_MTX  {Matrix} at 802327C0 ind 1  Load:Mod 
00229BB8: 01400040 00232740 CMD G_MTX  {Matrix} at 80232740 ind 1  Load:Mod 
00229BF0: 01400040 00232780 CMD G_MTX  {Matrix} at 80232780 ind 1  Load:Mod 
00229B28: 01000040 002326C0 CMD G_MTX  {Matrix} at 802326C0  Mul:Mod 
00229B78: 01400040 00232700 CMD G_MTX  {Matrix} at 80232700  Mul:Mod 
*/

// 0x80 seems to be mul
// 0x40 load


void RDP_GFX_MtxDKR(u32 dwCmd0, u32 dwCmd1)
{	
	u32 dwAddress = RDPSegAddr(dwCmd1);
	u32 dwCommand = (dwCmd0>>16)&0xFF;
	u32 dwLength  = (dwCmd0)    &0xFFFF;

	LONG nLoadCommand = dwCommand & G_MTX_LOAD ? D3DRENDER_LOAD : D3DRENDER_MUL;
	BOOL bPush = dwCommand & G_MTX_PUSH ? TRUE : FALSE;
	
	if (dwCommand == 0)
	{
	//	g_Renderer->ResetMatrices();
		nLoadCommand = D3DRENDER_LOAD;
	}

	if (dwCommand & 0x80)
	{
		nLoadCommand = D3DRENDER_MUL;
	}
	else
	{	
		nLoadCommand = D3DRENDER_LOAD;
	}
		nLoadCommand = D3DRENDER_LOAD;
//00229B00: BC000008 64009867 CMD G_MOVEWORD  Mem[8][00]=64009867 Fogrange 0.203..0.984 zmax=0.000000
//0x0021eef0: bc000008 64009867 G_MOVEWORD
	bPush = FALSE;
	if (dwAddress + 64 > g_dwRamSize)
	{
		DBGConsole_Msg(0, "Mtx: Address invalid (0x%08x)", dwAddress);
		return;
	}

	// Load matrix from dwAddress
	tMatrix mat;
	u32 dwI;
	u32 dwJ;
	const float fRecip = 1.0f / 65536.0f;
	
	for (dwI = 0; dwI < 4; dwI++) {
		for (dwJ = 0; dwJ < 4; dwJ++) {

			SHORT nDataHi = *(SHORT *)(g_pu8RamBase + ((dwAddress+(dwI<<3)+(dwJ<<1)     )^0x2));
			WORD  nDataLo = *(WORD  *)(g_pu8RamBase + ((dwAddress+(dwI<<3)+(dwJ<<1) + 32)^0x2));

			mat.m[dwI][dwJ] = (float)(((LONG)nDataHi<<16) | (nDataLo))*fRecip;
		}
	}

	//mat.m[3][0] = mat.m[3][1] = mat.m[3][2] = 0;
	//mat.m[3][3] = 1;


	g_Renderer->SetWorldView(mat, bPush, nLoadCommand);
	g_Renderer->PrintActive();
}

void RDP_GFX_PopMtx(u32 dwCmd0, u32 dwCmd1)
{
	BYTE nCommand = (BYTE)(dwCmd1 & 0xFF);

	// Do any of the other bits do anything?
	// So far only Extreme-G seems to Push/Pop projection matrices

	if (nCommand & G_MTX_PROJECTION)
	{
		g_Renderer->PopProjection();
	}
	else
	{
		g_Renderer->PopWorldView();
	}
}

/*
0016A748: DA380007 801764C8 CMD Zelda_LOADMTX  {Matrix} at 001764C8  Load:Prj 
Load matrix(1):
0016A750: DA380005 80176488 CMD Zelda_LOADMTX  {Matrix} at 00176488  Mul:Prj 
Mul matrix(1):
0016A760: DA380003 80176448 CMD Zelda_LOADMTX  {Matrix} at 00176448  Load:Mod 
Load matrix(0):
0016AA70: DA380000 80175F88 CMD Zelda_LOADMTX  {Matrix} at 00175F88  Mul:Mod Push 
Mul matrix(0): (push)


#define	G_MTX_MODELVIEW		0x00
#define	G_MTX_PROJECTION	0x01
#define	G_MTX_MUL			0x00
#define	G_MTX_LOAD			0x02
#define G_MTX_NOPUSH		0x00
#define G_MTX_PUSH			0x04
*/
#define G_ZMTX_MODELVIEW	0x00
#define G_ZMTX_PROJECTION	0x04
#define G_ZMTX_MUL			0x00
#define G_ZMTX_LOAD			0x02
#define G_ZMTX_PUSH			0x00
#define G_ZMTX_NOPUSH		0x01

void RDP_GFX_ZeldaMtx(u32 dwCmd0, u32 dwCmd1)
{	
	tMatrix mat;
	u32 dwI;
	u32 dwJ;
	const float fRecip = 1.0f / 65536.0f;
	u32 dwAddress = RDPSegAddr(dwCmd1);

	// THESE ARE SWAPPED OVER FROM NORMAL MTX
	// Not sure if this is right!!!!
	u32 dwCommand = (dwCmd0)&0xFF;
	u32 dwLength  = (dwCmd0 >> 8)&0xFFFF;

	if (dwAddress + 64 > g_dwRamSize)
	{
		DBGConsole_Msg(0, "ZeldaMtx: Address invalid (0x%08x)", dwAddress);
		return;
	}

	// Load matrix from dwAddress	
	for (dwI = 0; dwI < 4; dwI++)
	{
		for (dwJ = 0; dwJ < 4; dwJ++) 
		{
			SHORT nDataHi = *(SHORT *)(g_pu8RamBase + ((dwAddress+(dwI<<3)+(dwJ<<1)     )^0x2));
			WORD  nDataLo = *(WORD  *)(g_pu8RamBase + ((dwAddress+(dwI<<3)+(dwJ<<1) + 32)^0x2));

			mat.m[dwI][dwJ] = (float)(((LONG)nDataHi<<16) | (nDataLo))*fRecip;
		}
	}

	LONG nLoadCommand = dwCommand & G_ZMTX_LOAD ? D3DRENDER_LOAD : D3DRENDER_MUL;
	BOOL bPush = dwCommand & G_ZMTX_NOPUSH ? FALSE : TRUE;


	if (dwCommand & G_ZMTX_PROJECTION)
	{
		// So far only Extreme-G seems to Push/Pop projection matrices	
		g_Renderer->SetProjection(mat, bPush, nLoadCommand);
	}
	else
	{
		g_Renderer->SetWorldView(mat, bPush, nLoadCommand);
	}
}

void RDP_GFX_ZeldaPopMtx(u32 dwCmd0, u32 dwCmd1)
{
	BYTE nCommand = (BYTE)(dwCmd0 & 0xFF);

	g_Renderer->PopWorldView();
}

/*	
cc = cmd
ff = first
ll = len
          ccffll??
802C80C0: 01004008 01001C00 CMD Zelda_LOADVTX  Vertex 00..03 at 802C7480
80317030: 01020040 08000EB0 CMD Zelda_LOADVTX  Vertex 00..31 at 802048D0
80317140: 01010020 08001470 CMD Zelda_LOADVTX  Vertex 00..15 at 80204E90
80317228: 0100F01E 08001670 CMD Zelda_LOADVTX  Vertex 00..14 at 80205090
80315FE8: 0100D01A 0600A580 CMD Zelda_LOADVTX  Vertex 00..12 at 803158D0
80316060: 0101B036 0600A650 CMD Zelda_LOADVTX  Vertex 00..26 at 803159A0

802F9C48: 0100E024 0602D9E0 CMD Zelda_LOADVTX  Vertex 04..17 at 802F7150

*/
void RDP_GFX_Vtx_Zelda(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwAddress = RDPSegAddr(dwCmd1);
	u32 dwVEnd    = ((dwCmd0   )&0xFFF)/2;
	u32 dwN      = ((dwCmd0>>12)&0xFFF);

	u32 dwV0		= dwVEnd - dwN;

	if (dwV0 >= 32)
		dwV0 = 31;
	
	if ((dwV0 + dwN) > 32)
	{
        DBGConsole_Msg(0, "Warning, attempting to load into invalid vertex positions");
		dwN = 32 - dwV0;
	}

	// Check that address is valid...
	if ((dwAddress + (dwN*16)) > g_dwRamSize)
	{
		DBGConsole_Msg(0, "SetNewVertexInfo: Address out of range (0x%08x)", dwAddress);
	}
	else
	{
		g_Renderer->SetNewVertexInfo(dwAddress, dwV0, dwN);

		g_dwNumVertices += dwN;

		RDP_GFX_DumpVtxInfo(dwAddress, dwV0, dwN);
	}

}

// 12
/*
                08                            00..00
00208B10: 0400001A 000DD138 CMD G_VTX  Vertex 00..01 at 800DD138
                2C                            00..02
002089F8: 0416003E 00248B06 CMD G_VTX  Vertex 00..03 at 80248B06
002089A8: 041E0050 00248A66 CMD G_VTX  Vertex 00..04 at 80248A66
002088C0: 04240062 00247EA4 CMD G_VTX  Vertex 00..05 at 80247EA4
00208970: 04280074 00248318 CMD G_VTX  Vertex 00..06 at 80248318
00208948: 04300086 00248778 CMD G_VTX  Vertex 00..07 at 80248778
00208958: 04380098 002482C8 CMD G_VTX  Vertex 00..08 at 802482C8
002088F8: 044000AA 00247FA8 CMD G_VTX  Vertex 00..09 at 80247FA8

00229DA8: 045000CE 002497E0 CMD G_VTX  Vertex 00..11 at 802497E0
002089B0: 045E00E0 002483FE CMD G_VTX  Vertex 00..12 at 802483FE
80208BD8: 046200F2 002292BA CMD G_VTX  Vertex 00..13 at 802292BA
00208898: 046C0104 00247D64 CMD G_VTX  Vertex 00..14 at 80247D64
00208900: 04760116 0024853E CMD G_VTX  Vertex 00..15 at 8024853E
00208920: 047A0128 002480A2 CMD G_VTX  Vertex 00..16 at 802480A2
80229F90: 0486013A 00228F86 CMD G_VTX  Vertex 00..17 at 80228F86
00229CC8: 0488014C 00248BD8 CMD G_VTX  Vertex 00..18 at 80248BD8
00208930: 0492015E 00248142 CMD G_VTX  Vertex 00..19 at 80248142
00208940: 04980170 00248200 CMD G_VTX  Vertex 00..20 at 80248200
80208B68: 04A40182 00228EB4 CMD G_VTX  Vertex 00..21 at 80228EB4

80208BB8: 04B001A6 00229148 CMD G_VTX  Vertex 00..23 at 80229148

00229BA0: 04280074 001E4B70 CMD G_VTX  Vertex 20..19 at 801E4B70

*/

/*
 0     1    2   3    4     5    6   7    8    9
 y1   x1   a1   z1   x2   b1   z2   y2   a2   b2
ff00 2831 ffff fc7c 1ee8 ffff 19ee ff00 ffff ffff
11a1 2831 ffff fc7c 1ee8 ffff 19ee 11a1 ffff ffff
11a1 19ee ffff e118 19ee ffff e118 ff00 ffff ffff
11a1 d7cf ffff 0384 e118 ffff e612 ff00 ffff ffff
11a1 e118 ffff e612 d7cf ffff 0384 ff00 ffff ffff
*/

void RDP_GFX_Vtx_DKR(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwAddress = RDPSegAddr(dwCmd1);
	u32 dwV0 =  0;
	u32 dwN  = ((dwCmd0 & 0xFFF) - 0x08) / 0x12;

	if (dwV0 >= 32)
		dwV0 = 31;
	
	if ((dwV0 + dwN) > 32)
	{
		DBGConsole_Msg(0, "Warning, attempting to load into invalid vertex positions");
		dwN = 32 - dwV0;
	}

	// Check that address is valid...
	if ((dwAddress + (dwN*16)) > g_dwRamSize)
	{
		DBGConsole_Msg(0, "SetNewVertexInfo: Address out of range (0x%08x)", dwAddress);
	}
	else
	{
		g_Renderer->SetNewVertexInfoDKR(dwAddress, dwV0, dwN);

		g_dwNumVertices += dwN;

		RDP_GFX_DumpVtxInfoDKR(dwAddress, dwV0, dwN);
	}
}


void RDP_GFX_Vtx_Mario(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwAddress = RDPSegAddr(dwCmd1);
	u32 dwV0 =  (dwCmd0>>16)&0x0F;
	u32 dwN  = ((dwCmd0>>20)&0x0F)+1;
	u32 dwLength = (dwCmd0)&0xFFFF;

	if (dwV0 >= 32)
		dwV0 = 31;
	
	if ((dwV0 + dwN) > 32)
	{
		DBGConsole_Msg(0, "Warning, attempting to load into invalid vertex positions");
		dwN = 32 - dwV0;
	}

	// Check that address is valid...
	if ((dwAddress + (dwN*16)) > g_dwRamSize)
	{
		DBGConsole_Msg(0, "SetNewVertexInfo: Address out of range (0x%08x)", dwAddress);
	}
	else
	{
		g_Renderer->SetNewVertexInfo(dwAddress, dwV0, dwN);

		g_dwNumVertices += dwN;

		RDP_GFX_DumpVtxInfo(dwAddress, dwV0, dwN);
	}
}

void RDP_GFX_Vtx_MarioKart(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwAddress = RDPSegAddr(dwCmd1);
	u32 dwLength = (dwCmd0)&0xFFFF;

	u32 dwN= (dwLength + 1) / 0x410;
	u32 dwV0 = ((dwCmd0>>16)&0x3f)>>1;


	if (dwAddress > g_dwRamSize)
	{
		return;
	}

	if (dwV0 >= 32)
		dwV0 = 31;
	
	if ((dwV0 + dwN) > 32)
	{
		DL_PF("        Warning, attempting to load into invalid vertex positions");
		dwN = 32 - dwV0;
	}

	g_Renderer->SetNewVertexInfo(dwAddress, dwV0, dwN);

	g_dwNumVertices += dwN;

	RDP_GFX_DumpVtxInfo(dwAddress, dwV0, dwN);
}

void RDP_GFX_Vtx_WaveraceUS(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwAddress = RDPSegAddr(dwCmd1);
	u32 dwLength = (dwCmd0)&0xFFFF;

	u32 dwN= (dwLength + 1) / 0x210;
	//u32 dwV0 = ((dwCmd0>>16)&0x3f)/5;
	u32 dwV0 = ((dwCmd0>>16)&0xFF)/5;

	DL_PF("    Address 0x%08x, v0: %d, Num: %d, Length: 0x%04x", dwAddress, dwV0, dwN, dwLength);

	if (dwV0 >= 32)
		dwV0 = 31;
	
	if ((dwV0 + dwN) > 32)
	{
		DL_PF("    *Warning, attempting to load into invalid vertex positions");
		DBGConsole_Msg(0, "Warning, attempting to load into invalid vertex positions");
		dwN = 32 - dwV0;
	}

	g_Renderer->SetNewVertexInfo(dwAddress, dwV0, dwN);

	g_dwNumVertices += dwN;

	RDP_GFX_DumpVtxInfo(dwAddress, dwV0, dwN);
}



void RDP_GFX_DumpVtxInfo(u32 dwAddress, u32 dwV0, u32 dwN)
{
}
// DKR verts are extra 4 bytes
void RDP_GFX_DumpVtxInfoDKR(u32 dwAddress, u32 dwV0, u32 dwN)
{
}

void RDP_GFX_DmaTri(u32 dwCmd0, u32 dwCmd1)
{
//DmaTri: 0x05710080 0x0021a2a0
//DmaTri: 0x05e100f0 0x0021a100
//DmaTri: 0x05a100b0 0x0021a1f0
//00229BE0: 05710080 001E4CB0 CMD G_DMATRI  Triangles 9 at 801E4CB0
/*
00229CB8: 059100A0 002381A0 CMD G_DMATRI  Triangles 11 at 802381A0
tri[ 0 1 2 ] (0,1,2)
tri[ 2 3 0 ] (2,3,0)
tri[ 4 0 5 ] (4,0,5)
tri[ 5 6 4 ] (5,6,4)
tri[ 6 1 4 ] (6,1,4)
tri[ 7 8 9 ] (7,8,9)
tri[ 7 9 10 ] (7,9,10)
tri[ 10 11 7 ] (10,11,7)
tri[ 11 12 13 ] (11,12,13)
tri[ 13 8 11 ] (13,8,11)
tri[ 17 16 2 ] (17,16,2)

0x0022cda0: 00000102
0x0022cda4: 00000000
0x0022cda8: 00000000
0x0022cdac: 00000000
0x0022cdb0: 00020300
0x0022cdb4: 00000000
0x0022cdb8: 00000000
0x0022cdbc: 00000000
0x0022cdc0: 00040005

*/

	BOOL bTrisAdded = FALSE;
	u32 dwAddress = RDPSegAddr(dwCmd1);
	u32 dwNum = ((dwCmd0 & 0xFF) / 0x10);
	u32 i;
	u32 * pData = &g_pu32RamBase[dwAddress/4];

	for (i = 0; i < dwNum; i++)
	{
		DL_PF("    0x%08x: %08x %08x %08x %08x", dwAddress + i*16,
			pData[0], pData[1], pData[2], pData[3]);

		u32 dwInfo = pData[0];

		u32 dwV0 = (dwInfo >> 16) & 0x1F;
		u32 dwV1 = (dwInfo >>  8) & 0x1F;
		u32 dwV2 = (dwInfo      ) & 0x1F;


		if (g_Renderer->TestTri(dwV0, dwV1, dwV2))
		{
			DL_PF("   Tri: %d,%d,%d", dwV0, dwV1, dwV2);
			if (!bTrisAdded && g_bTextureEnable)
				RDP_EnableTexturing(g_dwTextureTile);
			bTrisAdded = TRUE;
			g_Renderer->AddTri(dwV0, dwV1, dwV2);
			g_dwNumTrisRendered++;
		}
		else
		{
			DL_PF("   Tri1: %d,%d,%d (clipped)", dwV0, dwV1, dwV2);
			g_dwNumTrisClipped++;
		}

		pData += 4;

	}

	if (bTrisAdded)	
	{
		g_Renderer->FlushTris();
	}
	else
	{
		g_dwNumTexturesIgnored++;
	}

}


/*
Zelda: Reserved0(0x022516ff 0x002814ff)
Zelda: Reserved0(0x022d18ff 0x004c22ff)
Zelda: Reserved0(0x024d2bff 0x187347ff)
SetNewVertexInfo: Address out of range (0x00c0f53f)
Zelda: Reserved0(0x022f1dff 0x085031ff)
SetNewVertexInfo: Address out of range (0x009c9b1f)
SetNewVertexInfo: Address out of range (0x00cc1e3f)
Zelda: Reserved0(0x02301cff 0x052618ff)
Zelda: Reserved0(0x020d1de2 0x0f1d3fff)
Warning, attempting to load into invalid vertex positions
Warning, attempting to load into invalid vertex positions
Warning, attempting to load into invalid vertex positions
Zelda: Reserved0(0x020815ff 0x05142eff)
Exception processing 0x05050300 0x00000000*/

void RDP_GFX_ZeldaTri1(u32 dwCmd0, u32 dwCmd1)
{
	// While the next command pair is Tri1, add vertices
	u32 dwPC = g_dwPCStack[g_dwPCStack.size() - 1].addr;
	u32 * pCmdBase = (u32 *)(g_pu8RamBase + dwPC);

	BOOL bTrisAdded = FALSE;
	
	do
	{
		//u32 dwFlag = (dwCmd1>>24)&0xFF;
		// Vertex indices are multiplied by 10 for Mario64, by 2 for MarioKart
		u32 dwV2 = ((dwCmd0>>16)&0xFF)/g_dwVertexMult;
		u32 dwV1 = ((dwCmd0>>8 )&0xFF)/g_dwVertexMult;
		u32 dwV0 = ((dwCmd0   )&0xFF)/g_dwVertexMult;


		if (g_Renderer->TestTri(dwV0, dwV1, dwV2))
		{
			DL_PF("    ZeldaTri1: 0x%08x 0x%08x %d,%d,%d", dwCmd0, dwCmd1, dwV0, dwV1, dwV2);
			if (!bTrisAdded && g_bTextureEnable)
				RDP_EnableTexturing(g_dwTextureTile);
			bTrisAdded = TRUE;
			g_Renderer->AddTri(dwV0, dwV1, dwV2);
			g_dwNumTrisRendered++;
		}
		else
		{
			DL_PF("    ZeldaTri1: 0x%08x 0x%08x %d,%d,%d (clipped)", dwCmd0, dwCmd1, dwV0, dwV1, dwV2);
			g_dwNumTrisClipped++;
		}

		dwCmd0			= *pCmdBase++;
		dwCmd1			= *pCmdBase++;
		dwPC += 8;

	} while ((dwCmd0>>24) == (BYTE)G_ZELDATRI1);
	g_dwPCStack[g_dwPCStack.size() - 1].addr = dwPC-8;


	if (bTrisAdded)	
	{
		g_Renderer->FlushTris();
	}
	else
	{
		g_dwNumTexturesIgnored++;
	}
}


//802C80C8: 06000204 00000406 CMD Zelda_TRI2  (2,1,0)
//00284030: 07002026 00002624 CMD Zelda_TRI3  (19,16,0)

void RDP_GFX_ZeldaTri2(u32 dwCmd0, u32 dwCmd1)
{
	// While the next command pair is Tri2, add vertices
	u32 dwPC = g_dwPCStack[g_dwPCStack.size() - 1].addr;

	BOOL bTrisAdded = FALSE;

	do {
		u32 dwV2 = ((dwCmd1>>16)&0xFF)/g_dwVertexMult;
		u32 dwV1 = ((dwCmd1>>8 )&0xFF)/g_dwVertexMult;
		u32 dwV0 = ((dwCmd1    )&0xFF)/g_dwVertexMult;

		u32 dwV5 = ((dwCmd0>>16)&0xFF)/g_dwVertexMult;
		u32 dwV4 = ((dwCmd0>>8 )&0xFF)/g_dwVertexMult;
		u32 dwV3 = ((dwCmd0    )&0xFF)/g_dwVertexMult;

		DL_PF("    ZeldaTri2: 0x%08x 0x%08x", dwCmd0, dwCmd1);
		DL_PF("           V0: %d, V1: %d, V2: %d", dwV0, dwV1, dwV2);
		DL_PF("           V3: %d, V4: %d, V5: %d", dwV3, dwV4, dwV5);

		// Do first tri
		if (g_Renderer->TestTri(dwV0, dwV1, dwV2))
		{
			if (!bTrisAdded && g_bTextureEnable)
				RDP_EnableTexturing(g_dwTextureTile);

			bTrisAdded = TRUE;
			g_Renderer->AddTri(dwV0, dwV1, dwV2);
			g_dwNumTrisRendered++;
		}
		else
			g_dwNumTrisClipped++;

		// Do second tri
		if (g_Renderer->TestTri(dwV3, dwV4, dwV5))
		{
			if (!bTrisAdded && g_bTextureEnable)
				RDP_EnableTexturing(g_dwTextureTile);

			bTrisAdded = TRUE;
			g_Renderer->AddTri(dwV3, dwV4, dwV5);
			g_dwNumTrisRendered++;
		}
		else
		{
			g_dwNumTrisClipped++;
		}
		
		dwCmd0			= *(u32 *)(g_pu8RamBase + dwPC+0);
		dwCmd1			= *(u32 *)(g_pu8RamBase + dwPC+4);
		dwPC += 8;

	} while ((dwCmd0>>24) == (BYTE)G_ZELDATRI2);


	g_dwPCStack[g_dwPCStack.size() - 1].addr = dwPC-8;


	if (bTrisAdded)	
	{
		g_Renderer->FlushTris();
	}
	else
	{
		g_dwNumTexturesIgnored++;
	}
}

//00284030: 07002026 00002624 CMD Zelda_TRI3  (19,16,0)
void RDP_GFX_ZeldaTri3(u32 dwCmd0, u32 dwCmd1)
{
	// While the next command pair is Tri2, add vertices
	u32 dwPC = g_dwPCStack[g_dwPCStack.size() - 1].addr;

	BOOL bTrisAdded = FALSE;

	do {
		// Vertex indices are multiplied by 10 for Mario64, by 2 for MarioKart
		u32 dwV2 = ((dwCmd1>>16)&0xFF)/g_dwVertexMult;
		u32 dwV1 = ((dwCmd1>>8 )&0xFF)/g_dwVertexMult;
		u32 dwV0 = ((dwCmd1    )&0xFF)/g_dwVertexMult;

		u32 dwV5 = ((dwCmd0>>16)&0xFF)/g_dwVertexMult;
		u32 dwV4 = ((dwCmd0>>8 )&0xFF)/g_dwVertexMult;
		u32 dwV3 = ((dwCmd0    )&0xFF)/g_dwVertexMult;

		DL_PF("    ZeldaTri3: 0x%08x 0x%08x", dwCmd0, dwCmd1);
		DL_PF("           V0: %d, V1: %d, V2: %d", dwV0, dwV1, dwV2);
		DL_PF("           V3: %d, V4: %d, V5: %d", dwV3, dwV4, dwV5);

		// Do first tri
		if (g_Renderer->TestTri(dwV0, dwV1, dwV2))
		{
			if (!bTrisAdded && g_bTextureEnable)
				RDP_EnableTexturing(g_dwTextureTile);
			bTrisAdded = TRUE;
			g_Renderer->AddTri(dwV0, dwV1, dwV2);
			g_dwNumTrisRendered++;
		}
		else
			g_dwNumTrisClipped++;

		// Do second tri
		if (g_Renderer->TestTri(dwV3, dwV4, dwV5))
		{
			if (!bTrisAdded && g_bTextureEnable)
				RDP_EnableTexturing(g_dwTextureTile);
			bTrisAdded = TRUE;
			g_Renderer->AddTri(dwV3, dwV4, dwV5);
			g_dwNumTrisRendered++;
		}
		else
			g_dwNumTrisClipped++;
		
		dwCmd0			= *(u32 *)(g_pu8RamBase + dwPC+0);
		dwCmd1			= *(u32 *)(g_pu8RamBase + dwPC+4);
		dwPC += 8;

	} while ((dwCmd0>>24) == (BYTE)G_ZELDATRI3);


	g_dwPCStack[g_dwPCStack.size() - 1].addr = dwPC-8;


	if (bTrisAdded)	
	{
		g_Renderer->FlushTris();
	}
	else
	{
		g_dwNumTexturesIgnored++;
	}
}


void RDP_GFX_Tri1(u32 dwCmd0, u32 dwCmd1)
{
	// While the next command pair is Tri1, add vertices
	u32 dwPC = g_dwPCStack[g_dwPCStack.size() - 1].addr;
	u32 * pCmdBase = (u32 *)(g_pu8RamBase + dwPC);
	
	BOOL bTrisAdded = FALSE;

	do
	{
		//u32 dwFlag = (dwCmd1>>24)&0xFF;
		// Vertex indices are multiplied by 10 for Mario64, by 2 for MarioKart
		u32 dwV0 = ((dwCmd1>>16)&0xFF)/g_dwVertexMult;
		u32 dwV1 = ((dwCmd1>>8 )&0xFF)/g_dwVertexMult;
		u32 dwV2 = ((dwCmd1    )&0xFF)/g_dwVertexMult;

		if (g_Renderer->TestTri(dwV0, dwV1, dwV2))
		{
			DL_PF("    Tri1: 0x%08x 0x%08x %d,%d,%d", dwCmd0, dwCmd1, dwV0, dwV1, dwV2);

			if (!bTrisAdded && g_bTextureEnable)
				RDP_EnableTexturing(g_dwTextureTile);

			bTrisAdded = TRUE;
			g_Renderer->AddTri(dwV0, dwV1, dwV2);
			g_dwNumTrisRendered++;
		}
		else
		{
			DL_PF("    Tri1: 0x%08x 0x%08x %d,%d,%d (clipped)", dwCmd0, dwCmd1, dwV0, dwV1, dwV2);
			g_dwNumTrisClipped++;
		}

		dwCmd0			= *pCmdBase++;
		dwCmd1			= *pCmdBase++;
		dwPC += 8;

	} while ((dwCmd0>>24) == (BYTE)G_TRI1);
	g_dwPCStack[g_dwPCStack.size() - 1].addr = dwPC-8;

	if (bTrisAdded)	
	{
		g_Renderer->FlushTris();
	}
	else
	{
		g_dwNumTexturesIgnored++;
	}

}

void RDP_GFX_Line3D(u32 dwCmd0, u32 dwCmd1)
{
	// While the next command pair is Tri1, add vertices
	u32 dwPC = g_dwPCStack[g_dwPCStack.size() - 1].addr;

	BOOL bTrisAdded = FALSE;


	do {
		u32 dwV3   = ((dwCmd1>>24)&0xFF)/g_dwVertexMult;
		u32 dwV0   = ((dwCmd1>>16)&0xFF)/g_dwVertexMult;
		u32 dwV1	 = ((dwCmd1>>8 )&0xFF)/g_dwVertexMult;
		u32 dwV2   = ((dwCmd1    )&0xFF)/g_dwVertexMult;

		DL_PF("    Line3D: V0: %d, V1: %d, V2: %d, V3: %d", dwV0, dwV1, dwV2, dwV3);

		// Do first tri
		if (g_Renderer->TestTri(dwV0, dwV1, dwV2))
		{
			if (!bTrisAdded && g_bTextureEnable)
				RDP_EnableTexturing(g_dwTextureTile);

			bTrisAdded = TRUE;
			g_Renderer->AddTri(dwV0, dwV1, dwV2);
			g_dwNumTrisRendered++;
		}
		else
			g_dwNumTrisClipped++;

		// Do second tri
		if (g_Renderer->TestTri(dwV2, dwV3, dwV0))
		{
			if (!bTrisAdded && g_bTextureEnable)
				RDP_EnableTexturing(g_dwTextureTile);

			bTrisAdded = TRUE;
			g_Renderer->AddTri(dwV2, dwV3, dwV0);
			g_dwNumTrisRendered++;
		}
		else
			g_dwNumTrisClipped++;


		dwCmd0			= *(u32 *)(g_pu8RamBase + dwPC+0);
		dwCmd1			= *(u32 *)(g_pu8RamBase + dwPC+4);
		dwPC += 8;

	} while ((dwCmd0>>24) == (BYTE)G_LINE3D);

	g_dwPCStack[g_dwPCStack.size() - 1].addr = dwPC-8;

	if (bTrisAdded)	
	{
		g_Renderer->FlushTris();
	}
	else
	{
		g_dwNumTexturesIgnored++;
	}	
}


void RDP_GFX_Tri2_Goldeneye(u32 dwCmd0, u32 dwCmd1)
{
	// While the next command pair is Tri2, add vertices
	u32 dwPC = g_dwPCStack[g_dwPCStack.size() - 1].addr;

	BOOL bTrisAdded = FALSE;

	do {
		// Vertex indices are exact
		u32 dwB = (dwCmd1    ) & 0xF;
		u32 dwA = (dwCmd1>> 4) & 0xF;
		u32 dwE = (dwCmd1>> 8) & 0xF;
		u32 dwD = (dwCmd1>>12) & 0xF;
		u32 dwH = (dwCmd1>>16) & 0xF;
		u32 dwG = (dwCmd1>>20) & 0xF;
		u32 dwK = (dwCmd1>>24) & 0xF;
		u32 dwJ = (dwCmd1>>28) & 0xF;

		u32 dwC = (dwCmd0    ) & 0xF;
		u32 dwF = (dwCmd0>> 4) & 0xF;
		u32 dwI = (dwCmd0>> 8) & 0xF;
		u32 dwL = (dwCmd0>>12) & 0xF;

		u32 dwFlag = (dwCmd0>>16)&0xFF;

		BOOL bVisible;

		DL_PF("    Tri2: 0x%08x 0x%08x Flag: 0x%02x", dwCmd0, dwCmd1, dwFlag);

		// Don't check the first two tris for nullity
		bVisible = g_Renderer->TestTri(dwA, dwC, dwB);
		DL_PF("       (%d, %d, %d) %s", dwA, dwB, dwC, bVisible ? "": "(clipped)");
		if (bVisible)
		{
			if (!bTrisAdded && g_bTextureEnable)
				RDP_EnableTexturing(g_dwTextureTile);

			bTrisAdded = TRUE;
			g_Renderer->AddTri(dwA, dwC, dwB);
			g_dwNumTrisRendered++;
		}
		else
		{
			g_dwNumTrisClipped++;
		}

		
		bVisible = g_Renderer->TestTri(dwD, dwF, dwE);
		DL_PF("       (%d, %d, %d) %s", dwD, dwE, dwF, bVisible ? "": "(clipped)");
		if (bVisible)
		{
			if (!bTrisAdded && g_bTextureEnable)
				RDP_EnableTexturing(g_dwTextureTile);

			bTrisAdded = TRUE;
			g_Renderer->AddTri(dwD, dwF, dwE);
			g_dwNumTrisRendered++;
		}
		else
		{
			g_dwNumTrisClipped++;
		}


		if (dwG != dwI && dwI != dwH && dwH != dwG)
		{
			bVisible = g_Renderer->TestTri(dwG, dwI, dwH);
			DL_PF("     (%d, %d, %d) %s", dwG, dwH, dwI, bVisible ? "": "(clipped)");
			if (bVisible)
			{
				if (!bTrisAdded && g_bTextureEnable)
					RDP_EnableTexturing(g_dwTextureTile);

				bTrisAdded = TRUE;
				g_Renderer->AddTri(dwG, dwI, dwH);
				g_dwNumTrisRendered++;
			}
			else
			{
				g_dwNumTrisClipped++;
			}
		}

		if (dwJ != dwL && dwL != dwK && dwK != dwJ)
		{
			bVisible = g_Renderer->TestTri(dwJ, dwL, dwK);
			DL_PF("     (%d, %d, %d) %s", dwJ, dwK, dwL, bVisible ? "": "(clipped)");
			if (bVisible)
			{
				if (!bTrisAdded && g_bTextureEnable)
					RDP_EnableTexturing(g_dwTextureTile);

				bTrisAdded = TRUE;
				g_Renderer->AddTri(dwJ, dwL, dwK);
				g_dwNumTrisRendered++;
			}
			else
			{
				g_dwNumTrisClipped++;
			}
		}
		
		dwCmd0			= *(u32 *)(g_pu8RamBase + dwPC+0);
		dwCmd1			= *(u32 *)(g_pu8RamBase + dwPC+4);
		dwPC += 8;

	} while ((dwCmd0>>24) == (BYTE)G_TRI2);


	g_dwPCStack[g_dwPCStack.size() - 1].addr = dwPC-8;


	if (bTrisAdded)	
	{
		g_Renderer->FlushTris();
	}		
	else
	{
		g_dwNumTexturesIgnored++;
	}
}



/* Mariokart etc*/
void RDP_GFX_Tri2_Mariokart(u32 dwCmd0, u32 dwCmd1)
{

	// While the next command pair is Tri2, add vertices
	u32 dwPC = g_dwPCStack[g_dwPCStack.size() - 1].addr;

	BOOL bTrisAdded = FALSE;

	do {
		// Vertex indices are multiplied by 10 for Mario64, by 2 for MarioKart
		u32 dwV0 = ((dwCmd1>>16)&0xFF)/g_dwVertexMult;
		u32 dwV1 = ((dwCmd1>>8 )&0xFF)/g_dwVertexMult;
		u32 dwV2 = ((dwCmd1    )&0xFF)/g_dwVertexMult;

		u32 dwV3 = ((dwCmd0>>16)&0xFF)/g_dwVertexMult;
		u32 dwV4 = ((dwCmd0>>8 )&0xFF)/g_dwVertexMult;
		u32 dwV5 = ((dwCmd0    )&0xFF)/g_dwVertexMult;

		DL_PF("    Tri2: 0x%08x 0x%08x", dwCmd0, dwCmd1);
		DL_PF("      V0: %d, V1: %d, V2: %d", dwV0, dwV1, dwV2);
		DL_PF("      V3: %d, V4: %d, V5: %d", dwV3, dwV4, dwV5);

		// Do first tri
		if (g_Renderer->TestTri(dwV0, dwV1, dwV2))
		{
			if (!bTrisAdded && g_bTextureEnable)
				RDP_EnableTexturing(g_dwTextureTile);

			bTrisAdded = TRUE;
			g_Renderer->AddTri(dwV0, dwV1, dwV2);
			g_dwNumTrisRendered++;
		}
		else
			g_dwNumTrisClipped++;

		// Do second tri
		if (g_Renderer->TestTri(dwV3, dwV4, dwV5))
		{
			if (!bTrisAdded && g_bTextureEnable)
				RDP_EnableTexturing(g_dwTextureTile);

			bTrisAdded = TRUE;
			g_Renderer->AddTri(dwV3, dwV4, dwV5);
			g_dwNumTrisRendered++;
		}
		else
		{
			g_dwNumTrisClipped++;
		}
		
		dwCmd0			= *(u32 *)(g_pu8RamBase + dwPC+0);
		dwCmd1			= *(u32 *)(g_pu8RamBase + dwPC+4);
		dwPC += 8;

	} while ((dwCmd0>>24) == (BYTE)G_TRI2);


	g_dwPCStack[g_dwPCStack.size() - 1].addr = dwPC-8;


	if (bTrisAdded)	
	{
		g_Renderer->FlushTris();
	}
	else
	{
		g_dwNumTexturesIgnored++;
	}
}


void RDP_GFX_SetScissor(u32 dwCmd0, u32 dwCmd1)
{
	// The coords are all in 8:2 fixed point
	g_Scissor.x0   = (dwCmd0>>12)&0xFFF;
	g_Scissor.y0   = (dwCmd0>>0 )&0xFFF;
	g_Scissor.mode = (dwCmd1>>24)&0x03;
	g_Scissor.x1   = (dwCmd1>>12)&0xFFF;
	g_Scissor.y1   = (dwCmd1>>0 )&0xFFF;

	DL_PF("    x0=%d y0=%d x1=%d y1=%d mode=%d",
		g_Scissor.x0/4, g_Scissor.y0/4,
		g_Scissor.x1/4, g_Scissor.y1/4,
		g_Scissor.mode);


	// Set the cliprect now...

	//RDP_NOIMPL_WARN("GFX_SetScissor Not Implemented");
}


void RDP_GFX_SetTile(u32 dwCmd0, u32 dwCmd1)
{
	char *pszImgFormat[8] = {"RGBA", "YUV", "CI", "IA", "I", "?1", "?2", "?3"};
	char *pszImgSize[4]   = {"4bpp", "8bpp", "16bpp", "32bpp"};
	char *pszOnOff[2]     = {"Off", "On"};

	u32 dwTile		= (dwCmd1>>24)&0x7;

	g_Tiles[dwTile].dwFormat	= (dwCmd0>>21)&0x7;
	g_Tiles[dwTile].dwSize		= (dwCmd0>>19)&0x3;
	g_Tiles[dwTile].dwLine		= (dwCmd0>>9 )&0x1FF;
	g_Tiles[dwTile].dwTMem		= (dwCmd0    )&0x1FF;

	g_Tiles[dwTile].dwPalette	= (dwCmd1>>20)&0x0F;
	g_Tiles[dwTile].bClampT		= (dwCmd1>>19)&0x01;
	g_Tiles[dwTile].bMirrorT	= (dwCmd1>>18)&0x01;
	g_Tiles[dwTile].dwMaskT		= (dwCmd1>>14)&0x0F;
	g_Tiles[dwTile].dwShiftT	= (dwCmd1>>10)&0x0F;		// LOD stuff
	g_Tiles[dwTile].bClampS		= (dwCmd1>>9 )&0x01;
	g_Tiles[dwTile].bMirrorS	= (dwCmd1>>8 )&0x01;
	g_Tiles[dwTile].dwMaskS		= (dwCmd1>>4 )&0x0F;
	g_Tiles[dwTile].dwShiftS	= (dwCmd1    )&0x0F;		// LOD stuff

	// Offset by dwTMem - LoadBlock.dwTMem
	g_Tiles[dwTile].dwAddress = g_TI.dwAddr;

	u32 dwLine = 0;
	switch(g_Tiles[dwTile].dwSize)
	{
		case G_IM_SIZ_4b:  dwLine = (g_Tiles[dwTile].dwLine*8); break;
		case G_IM_SIZ_8b:  dwLine = (g_Tiles[dwTile].dwLine*8); break;
		case G_IM_SIZ_16b: dwLine = (g_Tiles[dwTile].dwLine*8); break;
		case G_IM_SIZ_32b: dwLine = (g_Tiles[dwTile].dwLine*8)*2; break;
	}

	g_Tiles[dwTile].dwPitch = dwLine;

	DL_PF("    Tile:%d  Fmt: %s/%s Line:%d (Pitch %d) TMem:0x%04x Palette:%d",
		dwTile, pszImgFormat[g_Tiles[dwTile].dwFormat], pszImgSize[g_Tiles[dwTile].dwSize],
		g_Tiles[dwTile].dwLine, dwLine, g_Tiles[dwTile].dwTMem, g_Tiles[dwTile].dwPalette);
	DL_PF("         S: Clamp: %s Mirror:%s Mask:0x%x Shift:0x%x",
		pszOnOff[g_Tiles[dwTile].bClampS],pszOnOff[g_Tiles[dwTile].bMirrorS],
		g_Tiles[dwTile].dwMaskS, g_Tiles[dwTile].dwShiftS);
	DL_PF("         T: Clamp: %s Mirror:%s Mask:0x%x Shift:0x%x",
		pszOnOff[g_Tiles[dwTile].bClampT],pszOnOff[g_Tiles[dwTile].bClampT],
		g_Tiles[dwTile].dwMaskT, g_Tiles[dwTile].dwShiftT);

}

void RDP_GFX_SetTileSize(u32 dwCmd0, u32 dwCmd1)
{
	static BOOL bSkipNext = FALSE;
	u32 dwTile	= (dwCmd1>>24)&0x07;
		
	u32 nULS		= (u32)(((dwCmd0>>12)&0x0FFF) << 20) >> 20;
	u32 nULT		= (u32)(((dwCmd0    )&0x0FFF) << 20) >> 20;
	u32 nLRS		= (u32)(((dwCmd1>>12)&0x0FFF) << 20) >> 20;
	u32 nLRT		= (u32)(((dwCmd1    )&0x0FFF) << 20) >> 20;

	DL_PF("    Tile:%d (%d,%d) -> (%d,%d) [%d x %d]",
		dwTile, nULS/4, nULT/4,
		        nLRS/4, nLRT/4,
				((nLRS/4) - (nULS/4)) + 1,
				((nLRT/4) - (nULT/4)) + 1);

	if (bSkipNext)
	{
		bSkipNext = FALSE;

		DL_PF("Tetrisphere hack! - skipping command");
		return;
	}

	// Wetrix hack
	if ((LONG)nULS < 0 || (LONG)nLRS < 0 ||
		(LONG)nULT < 0 || (LONG)nLRT < 0)
	{
		DL_PF("             Ignoring - negative offsets");
		return;
	}
	if (nULT > nLRT || nULS > nLRS)
	{
		DL_PF("             Ignoring - negative size");
		return;
	}

	g_Tiles[dwTile].nULS = nULS / 4;
	g_Tiles[dwTile].nULT = nULT / 4;
	g_Tiles[dwTile].nLRS = nLRS / 4;
	g_Tiles[dwTile].nLRT = nLRT / 4;


	if ((LONG)g_Tiles[dwTile].nULS < 0)
	{
		g_Tiles[dwTile].nLRS -= g_Tiles[dwTile].nULS;
		g_Tiles[dwTile].nULS = 0;
	}
	if ((LONG)g_Tiles[dwTile].nULT < 0)
	{
		g_Tiles[dwTile].nLRT -= g_Tiles[dwTile].nULT;
		g_Tiles[dwTile].nULT = 0;
	}


	g_Tiles[dwTile].nLRS++;
	g_Tiles[dwTile].nLRT++;

	if (g_Tiles[dwTile].nLRS < g_Tiles[dwTile].nULS)
	{
		LONG nTemp = g_Tiles[dwTile].nLRS;
		g_Tiles[dwTile].nLRS = g_Tiles[dwTile].nULS;
		g_Tiles[dwTile].nULS = nTemp;
		
	}

	if (g_Tiles[dwTile].nLRT < g_Tiles[dwTile].nULT)
	{
		LONG nTemp = g_Tiles[dwTile].nLRT;
		g_Tiles[dwTile].nLRT = g_Tiles[dwTile].nULT;
		g_Tiles[dwTile].nULT = nTemp;

	}

	// Testrisphere hack - ignore second SetTileSize
	//SetTileSize: 0xf20001b0 0x004fc1c4 Tile:0 (0,108) -> (319,113) [320 x 6]
	//             (0,432) -> (1276, 452) [1277 x 21]
	//SetTileSize: 0xf2000000 0x004ff017 Tile:0 (0,0) -> (319,5) [320 x 6]
	//             (0,0) -> (1279, 23) [1280 x 24]


	u32 dwPC = g_dwPCStack[g_dwPCStack.size() - 1].addr;

	u32 dwNextCmd0			= *(u32 *)(g_pu8RamBase + dwPC+0);
	u32 dwNextCmd1			= *(u32 *)(g_pu8RamBase + dwPC+4);

	if ((dwNextCmd0>>24) == (BYTE)G_SETTILESIZE &&
		(dwNextCmd1>>24) == dwTile)
	{
		// Skip it!
		//g_dwPCStack[g_dwPCStack.size() - 1].addr += 8;
		bSkipNext = TRUE;
	}

	

}


void RDP_GFX_SetTImg(u32 dwCmd0, u32 dwCmd1)
{
	static const char *pszImgFormat[8] = {"RGBA", "YUV", "CI", "IA", "I", "?1", "?2", "?3"};
	static const char *pszImgSize[4] = {"4", "8", "16", "32"};

	g_TI.dwFormat = (dwCmd0>>21)&0x7;
	g_TI.dwSize   = (dwCmd0>>19)&0x3;
	g_TI.dwWidth  = (dwCmd0&0x0FFF) + 1;
	g_TI.dwAddr   = RDPSegAddr(dwCmd1);

	DL_PF("    Image: 0x%08x Fmt: %s/%s Width: %d", g_TI.dwAddr,
		pszImgFormat[g_TI.dwFormat], pszImgSize[g_TI.dwSize], g_TI.dwWidth);
}


void RDP_GFX_LoadBlock(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwULS		= ((dwCmd0>>12)&0x0FFF)/4;
	u32 dwULT		= ((dwCmd0    )&0x0FFF)/4;
	u32 dwPad     = (dwCmd1>>27)&0x1f;
	u32 dwTile	= (dwCmd1>>24)&0x07;
	u32 dwPixels	= ((dwCmd1>>12)&0x0FFF) + 1;		// Number of bytes-1?
	u32 dwDXT		= (dwCmd1    )&0x0FFF;		// 1.11 fixed point

	u32 dwQWs;

	// Ignore second texture stuff - this isn't handled at the moment
	if (g_Tiles[7].dwTMem > 0)
		return;
	
	if (dwDXT == 0)
	{
		g_bDXTZero = TRUE;
		dwQWs = 1;
	}
	else
	{
		g_bDXTZero = FALSE;
		dwQWs = 2048 / dwDXT;						// #Quad Words
	}
	u32 dwBytes = dwQWs * 8;
	u32 dwWidth = (dwBytes*2)>>g_TI.dwSize;

	DL_PF("    Tile:%d (%d,%d) %d pixels DXT:0x%04x = %d QWs => %d pixels/line",
		dwTile, dwULS, dwULT, dwPixels, dwDXT, dwQWs, dwWidth);

	u32 dwPixelOffset = (dwWidth * dwULT) + dwULS;
	u32 dwOffset = (dwPixelOffset<<g_TI.dwSize)/2;

	u32 dwSrcOffset = (g_TI.dwAddr + dwOffset);

	DL_PF("    Offset: 0x%08x", dwSrcOffset);

	// The N64 would load the texture into memory now
	g_nLastLoad = LOAD_BLOCK;
	g_dwLoadAddress = dwSrcOffset;
	g_dwLoadTilePitch = ~0;


}

void RDP_GFX_LoadTile(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwULS		= ((dwCmd0>>12)&0x0FFF)/4;
	u32 dwULT		= ((dwCmd0    )&0x0FFF)/4;
	u32 dwTile	=  (dwCmd1>>24)&0x07;
	u32 dwLRS		= ((dwCmd1>>12)&0x0FFF)/4;
	u32 dwLRT		= ((dwCmd1    )&0x0FFF)/4;

	// Todo - check if this is odd, and round up?
	u32 dwPitch = ((g_TI.dwWidth<<g_TI.dwSize)+1)/2;
	u32 dwOffset = (dwPitch* dwULT) + dwULS;
	u32 dwSrcOffset = (g_TI.dwAddr + dwOffset);
	
	DL_PF("    Tile:%d (%d,%d) -> (%d,%d) [%d x %d]",
		dwTile, dwULS, dwULT, dwLRS, dwLRT,
		(dwLRS - dwULS)+1, (dwLRT - dwULT)+1);
	DL_PF("    Actual: 0x%08x (LoadTile Pitch: %d)",
		dwSrcOffset, dwPitch);

	g_nLastLoad = LOAD_TILE;
	g_dwLoadAddress = g_TI.dwAddr; 
	g_dwLoadTilePitch = dwPitch;

}

void RDP_GFX_TexRectFlip(u32 dwCmd0, u32 dwCmd1)
{ 

	// This command used 128bits, and not 64 bits. This means that we have to look one 
	// Command ahead in the buffer, and update the PC.
	u32 dwPC = g_dwPCStack[g_dwPCStack.size() - 1].addr;		// This points to the next instruction
	u32 dwCmd2 = *(u32 *)(g_pu8RamBase + dwPC+4);
	u32 dwCmd3 = *(u32 *)(g_pu8RamBase + dwPC+4+8);

	// Increment PC so that it points to the right place
	g_dwPCStack[g_dwPCStack.size() - 1].addr += 16;

	u32 dwXH		= ((dwCmd0>>12)&0x0FFF)/4;
	u32 dwYH		= ((dwCmd0    )&0x0FFF)/4;
	u32 dwTile	= (dwCmd1>>24)&0x07;
	u32 dwXL		= ((dwCmd1>>12)&0x0FFF)/4;
	u32 dwYL		= ((dwCmd1    )&0x0FFF)/4;
	u32 dwS		= (  dwCmd2>>16)&0xFFFF;
	u32 dwT		= (  dwCmd2    )&0xFFFF;
	LONG  nDSDX 	= (LONG)(SHORT)((  dwCmd3>>16)&0xFFFF);
	LONG  nDTDY	    = (LONG)(SHORT)((  dwCmd3    )&0xFFFF);

	float fS0 = (float)dwS / 32.0f;
	float fT0 = (float)dwT / 32.0f;

	float fDSDX = (float)nDSDX / 1024.0f;
	float fDTDY = (float)nDTDY / 1024.0f;

	if ((g_dwOtherModeH & G_CYC_COPY) == G_CYC_COPY)
	{
		fDSDX /= 4.0f;	// In copy mode 4 pixels are copied at once.
	}
	if (g_bIncTexRectEdge)
	{
		dwXH++;
		dwYH++;
	}

	float fS1 = fS0 + (fDSDX * (dwYH - dwYL));
	float fT1 = fT0 + (fDTDY * (dwXH - dwXL));
	
	DL_PF("    Tile:%d (%d,%d) -> (%d,%d)",
		dwTile, dwXL, dwYL, dwXH, dwYH);
	DL_PF("    Tex:(%#5f,%#5f) -> (%#5f,%#5f) (DSDX:%#5f DTDY:%#5f)",
		fS0, fT0, fS1, fT1, fDSDX, fDTDY);
	DL_PF("");

	RDP_EnableTexturing(dwTile);
	
	g_Renderer->TexRectFlip(dwXL, dwYL, dwXH, dwYH, fS0, fT0, fS1, fT1);

	g_dwNumTrisRendered += 2;


}


void RDP_GFX_TexRect(u32 dwCmd0, u32 dwCmd1)
{
	// This command used 128bits, and not 64 bits. This means that we have to look one 
	// Command ahead in the buffer, and update the PC.
	u32 dwPC = g_dwPCStack[g_dwPCStack.size() - 1].addr;		// This points to the next instruction
	u32 dwCmd2 = *(u32 *)(g_pu8RamBase + dwPC+4);
	u32 dwCmd3 = *(u32 *)(g_pu8RamBase + dwPC+4+8);

	// Increment PC so that it points to the right place
	g_dwPCStack[g_dwPCStack.size() - 1].addr += 16;

	DL_PF("0x%08x: %08x %08x", dwPC, *(u32 *)(g_pu8RamBase + dwPC+0), *(u32 *)(g_pu8RamBase + dwPC+4));
	DL_PF("0x%08x: %08x %08x", dwPC+8, *(u32 *)(g_pu8RamBase + dwPC+8), *(u32 *)(g_pu8RamBase + dwPC+8+4));

	u32 dwXH		= ((dwCmd0>>12)&0x0FFF)/4;
	u32 dwYH		= ((dwCmd0    )&0x0FFF)/4;
	u32 dwTile	= (dwCmd1>>24)&0x07;
	u32 dwXL		= ((dwCmd1>>12)&0x0FFF)/4;
	u32 dwYL		= ((dwCmd1    )&0x0FFF)/4;
	u32 dwS		= (  dwCmd2>>16)&0xFFFF;
	u32 dwT		= (  dwCmd2    )&0xFFFF;
	LONG  nDSDX 	= (LONG)(SHORT)((  dwCmd3>>16)&0xFFFF);
	LONG  nDTDY	    = (LONG)(SHORT)((  dwCmd3    )&0xFFFF);

	float fS0 = (float)dwS / 32.0f;
	float fT0 = (float)dwT / 32.0f;

	float fDSDX = (float)nDSDX / 1024.0f;
	float fDTDY = (float)nDTDY / 1024.0f;

	if ((g_dwOtherModeH & G_CYC_COPY) == G_CYC_COPY)
	{
		fDSDX /= 4.0f;	// In copy mode 4 pixels are copied at once.
	}

	if (g_bIncTexRectEdge)
	{
		dwXH++;
		dwYH++;
	}

	// Offset to start of this tile!
	// HACK! For Testrisphere, TexCoords are set to zero by default
	if (dwS != 0)
		fS0 -= (float)g_Tiles[dwTile].nULS;
	if (dwT != 0)
		fT0 -= (float)g_Tiles[dwTile].nULT;

	float fS1 = fS0 + (fDSDX * (dwXH - dwXL));
	float fT1 = fT0 + (fDTDY * (dwYH - dwYL));

	DL_PF("    Tile:%d Screen(%d,%d) -> (%d,%d)", dwTile, dwXL, dwYL, dwXH, dwYH);
	DL_PF("           Tex:(%#5f,%#5f) -> (%#5f,%#5f) (DSDX:%#5f DTDY:%#5f)",
                                            fS0, fT0, fS1, fT1, fDSDX, fDTDY);
	DL_PF("");

	RDP_EnableTexturing(dwTile);

	g_Renderer->TexRect(dwXL, dwYL, dwXH, dwYH, fS0, fT0, fS1, fT1);

	g_dwNumTrisRendered += 2;


}



void RDP_GFX_FillRect(u32 dwCmd0, u32 dwCmd1)
{ 
	u32 x0   = ((dwCmd1>>12)&0xFFF)/4;
	u32 y0   = ((dwCmd1>>0 )&0xFFF)/4;
	u32 x1   = ((dwCmd0>>12)&0xFFF)/4;
	u32 y1   = ((dwCmd0>>0 )&0xFFF)/4;

	// Note, in some modes, the right/bottom lines aren't drawn

	DL_PF("    (%d,%d) (%d,%d)", x0, y0, x1, y1);

	// TODO - In 1/2cycle mode, skip bottom/right edges!?

	if (g_DI.dwAddr == g_CI.dwAddr)
	{
		// Clear the Z Buffer
		//s_pD3DX->SetClearDepth(depth from set fill color);
		//s_pD3DX->Clear(D3DCLEAR_ZBUFFER);

		DL_PF("    Clearing ZBuffer");

	}
	else
	{
		// Clear the screen if large rectangle?
		{

			/*if (((x1+1)-x0) == g_dwViWidth &&
				((y1+1)-y0) == g_dwViHeight &&
				(g_dwFillColor & 0x00FFFFFF) == 0x00000000)
			{
				DL_PF("  Filling Rectangle (screen clear)");
			//	s_pD3DX->Clear(D3DCLEAR_TARGET);

				//DBGConsole_Msg(0, "DL ColorBuffer = 0x%08x", g_CI.dwAddr);
			}
			else*/
			{
				DL_PF("    Filling Rectangle");
				g_Renderer->FillRect(x0, y0, x1+1, y1+1, g_dwFillColor);
			}
		}

	}


}


void RDP_GFX_Texture(u32 dwCmd0, u32 dwCmd1)
{
	//u32 dwBowTie = (dwCmd0>>16)&0xFF;		// Don't care about this
	u32 dwLevel  = (dwCmd0>>11)&0x07;
	g_dwTextureTile= (dwCmd0>>8 )&0x07;

	BOOL bTextureEnable          = (dwCmd0    )&0x01;
	float fTextureScaleS = (float)((dwCmd1>>16)&0xFFFF) / (65536.0f * 32.0f);
	float fTextureScaleT = (float)((dwCmd1    )&0xFFFF) / (65536.0f * 32.0f);

	g_Renderer->SetTextureEnable(bTextureEnable);
	g_Renderer->SetTextureScale(fTextureScaleS, fTextureScaleT);

	// What happens if these are 0? Interpret as 1.0f?

	DL_PF("    Level: %d Tile: %d %s", dwLevel, g_dwTextureTile, bTextureEnable ? "enabled":"disabled");
	DL_PF("    ScaleS: %f, ScaleT: %f", fTextureScaleS*32.0f, fTextureScaleT*32.0f);

	g_bTextureEnable = bTextureEnable;

}

//802FA128: D7000002 FFFFFFFF CMD Zelda_TEXTURE  texture on=2 tile=0 s=1.00 t=1.00
//000F8AA8: D7000000 FFFFFFFF CMD Zelda_TEXTURE  texture on=0 tile=0 s=1.00 t=1.00

// NOT CHECKED!
void RDP_GFX_ZeldaTexture(u32 dwCmd0, u32 dwCmd1)
{
	//u32 dwBowTie = (dwCmd0>>16)&0xFF;		// Don't care about this
	u32 dwLevel  = (dwCmd0>>11)&0x07;
	g_dwTextureTile= (dwCmd0>>8 )&0x07;

	BOOL bTextureEnable          = (dwCmd0    )&0x02;		// Important! not 0x01!?!?
	float fTextureScaleS = (float)((dwCmd1>>16)&0xFFFF) / (65536.0f * 32.0f);
	float fTextureScaleT = (float)((dwCmd1    )&0xFFFF) / (65536.0f * 32.0f);

	g_Renderer->SetTextureEnable(bTextureEnable);
	g_Renderer->SetTextureScale(fTextureScaleS, fTextureScaleT);

	// What happens if these are 0? Interpret as 1.0f?

	DL_PF("    Level: %d Tile: %d %s", dwLevel, g_dwTextureTile, bTextureEnable ? "enabled":"disabled");
	DL_PF("    ScaleS: %f, ScaleT: %f", fTextureScaleS*32.0f, fTextureScaleT*32.0f);


	g_bTextureEnable = bTextureEnable;

}

void RDP_GFX_SetZImg(u32 dwCmd0, u32 dwCmd1)
{
	DL_PF("    Image: 0x%08x", RDPSegAddr(dwCmd1));

	g_DI.dwAddr = RDPSegAddr(dwCmd1);
}

void RDP_GFX_SetCImg(u32 dwCmd0, u32 dwCmd1) {

	static const char *pszImgFormat[8] = {"RGBA", "YUV", "CI", "IA", "I", "?1", "?2", "?3"};
	static const char *pszImgSize[4] = {"4", "8", "16", "32"};

	u32 dwFmt   = (dwCmd0>>21)&0x7;
	u32 dwSiz   = (dwCmd0>>19)&0x3;
	u32 dwWidth = (dwCmd0&0x0FFF) + 1;

	DL_PF("    Image: 0x%08x", RDPSegAddr(dwCmd1));
	DL_PF("    Fmt: %s Size: %s Width: %d",
		pszImgFormat[dwFmt], pszImgSize[dwSiz], dwWidth);

	g_CI.dwAddr = RDPSegAddr(dwCmd1);
	g_CI.dwFormat = dwFmt;
	g_CI.dwSize = dwSiz;
	g_CI.dwWidth = dwWidth;
}



/*
// a or c
#define	G_BL_CLR_IN		0
#define	G_BL_CLR_MEM	1
#define	G_BL_CLR_BL		2
#define	G_BL_CLR_FOG	3

//b
#define	G_BL_A_IN		0
#define	G_BL_A_FOG		1
#define	G_BL_A_SHADE	2
#define	G_BL_0			3

//d
#define	G_BL_1MA		0
#define	G_BL_A_MEM		1
#define	G_BL_1			2
*/

static const char * sc_szBlClr[4] = { "In", "Mem", "Bl", "Fog" };
static const char * sc_szBlA1[4] = { "AIn", "AFog", "AShade", "0" };
static const char * sc_szBlA2[4] = { "1-A", "AMem", "1", "?" };
/*
#define	GBL_c1(m1a, m1b, m2a, m2b)	\
	(m1a) << 30 | (m1b) << 26 | (m2a) << 22 | (m2b) << 18
#define	GBL_c2(m1a, m1b, m2a, m2b)	\
	(m1a) << 28 | (m1b) << 24 | (m2a) << 20 | (m2b) << 16
*/

/*
  Blend: 0x0050: In*AIn + Mem*AIn    | In*AIn + Mem*AIn
  Blend: 0x0150: In*AIn + Mem*AIn    | In*AFog + Mem*AIn
  Blend: 0x0c18: In*0 + In*AShade    | In*AIn + Mem*AIn
  Blend: 0x0c19: In*0 + In*AShade    | In*AIn + Mem*AFog
  Blend: 0xc810: Fog*AShade + In*AIn | In*AIn + Mem*AIn
  Blend: 0xc811: Fog*AShade + In*AIn | In*AIn + Mem*AFog
*/


void RDP_GFX_SetOtherMode_L(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwShift = (dwCmd0>>8)&0xFF;
	u32 dwLength= (dwCmd0   )&0xFF;
	u32 dwData  = dwCmd1;

	u32 dwMask = ((1<<dwLength)-1)<<dwShift;

	// Mask off bits that don't apply. Might not be necessary
	//dwData &= dwMask;
	
	g_dwOtherModeL = (g_dwOtherModeL&(~dwMask)) | dwData;

	BOOL bZCompare		= (g_dwOtherModeL & Z_CMP)			? TRUE : FALSE;
	BOOL bZUpdate		= (g_dwOtherModeL & Z_UPD)			? TRUE : FALSE;
	
	g_Renderer->SetZCompare( bZCompare );
	g_Renderer->SetZUpdate( bZUpdate );
	

	//FIXME
	BOOL bZModeDecal    = (g_dwOtherModeL & ZMODE_DEC) == ZMODE_DEC     ? TRUE : FALSE;
	g_Renderer->SetZmodeDecal(bZModeDecal);

	BOOL bAA			= (g_dwOtherModeL & AA_EN) ? TRUE : FALSE;
	BOOL bZCmp			= (g_dwOtherModeL & Z_CMP) ? TRUE : FALSE;
	BOOL bZUpd			= (g_dwOtherModeL & Z_UPD) ? TRUE : FALSE;
	BOOL bImRd			= (g_dwOtherModeL & IM_RD) ? TRUE : FALSE;
	BOOL bColorOnCvg	= (g_dwOtherModeL & CLR_ON_CVG) ? TRUE : FALSE;

	BOOL bCvgXAlpha		= (g_dwOtherModeL & CVG_X_ALPHA) ? TRUE : FALSE;
	BOOL bAlphaCvgSel	= (g_dwOtherModeL & ALPHA_CVG_SEL) ? TRUE : FALSE;
	BOOL bForceBl		= (g_dwOtherModeL & FORCE_BL) ? TRUE : FALSE;

	u32 dwCvgDstMode	= (g_dwOtherModeL >> 4) & 0x3;
	u32 dwZMode		= (g_dwOtherModeL >> 6) & 0x3;

	u32 dwAlphaCompType = (g_dwOtherModeL >> G_MDSFT_ALPHACOMPARE)&0x3;
	u32 dwZSrcSel		= (g_dwOtherModeL >> G_MDSFT_ZSRCSEL)&0x1;

	u32 dwBlendType	= (g_dwOtherModeL >> 16) & 0xFFFF;

    // Alpha-Testing
    u32 dwAlphaTestMode = (g_dwOtherModeL >> G_MDSFT_ALPHACOMPARE) & 0x3;
	if(dwAlphaTestMode == G_AC_THRESHOLD && !bAlphaCvgSel)
        g_Renderer->SetAlphaTestEnable(TRUE);
	else if(bCvgXAlpha)
		g_Renderer->SetAlphaTestEnable(TRUE);
	else
		g_Renderer->SetAlphaTestEnable(FALSE);

    // Alpha-Blending
    g_Renderer->SetAlphaBlendEnable(FALSE);
    if(bForceBl && g_Renderer->GetCycleType() != G_CYC_COPY && g_Renderer->GetCycleType() != G_CYC_FILL && !bAlphaCvgSel)
    {
        switch(g_dwOtherModeL >> 16)
        {
            case 0x0448: // Add
			case 0x055A:
                g_Renderer->SetAlphaBlendEnable(TRUE);
                g_Renderer->SetAlphaBlendFunc(PVR_BLEND_ONE, PVR_BLEND_ONE);
				break;
			case 0x0C08: // 1080 Sky
			case 0x0F0A: // Used LOTS of places
                /*KOS Zerstört Rendering in Super Mario 64, wenn aktiv*/
                //g_Renderer->SetAlphaBlendFunc(PVR_BLEND_ONE, PVR_BLEND_ZERO);
				break;
			case 0xC810: // Blends fog
			case 0xC811: // Blends fog
			case 0x0C18: // Standard interpolated blend
			case 0x0C19: // Used for antialiasing
			case 0x0050: // Standard interpolated blend
			case 0x0055: // Used for antialiasing
  	            g_Renderer->SetAlphaBlendEnable(TRUE);
				g_Renderer->SetAlphaBlendFunc(PVR_BLEND_SRCALPHA, PVR_BLEND_INVSRCALPHA);
				break;
			case 0x0FA5: // Seems to be doing just blend color - maybe combiner can be used for this?
			case 0x5055: // Used in Paper Mario intro, I'm not sure if this is right...
                g_Renderer->SetAlphaBlendEnable(TRUE);
			    g_Renderer->SetAlphaBlendFunc(PVR_BLEND_ZERO, PVR_BLEND_ONE);
				break;
			default:
                g_Renderer->SetAlphaBlendEnable(TRUE);
				g_Renderer->SetAlphaBlendFunc(PVR_BLEND_SRCALPHA, PVR_BLEND_INVSRCALPHA);
				break;
        }
    }
}



void RDP_GFX_ZeldaSetOtherMode_L(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwShift = (dwCmd0>>8)&0xFF;
	u32 dwLength= (dwCmd0   )&0xFF;
	u32 dwData  = dwCmd1;

	// Mask is constructed slightly differently
	u32 dwMask = (u32)((s32)(0x80000000)>>dwLength)>>dwShift;

	dwData &= dwMask;

	g_dwOtherModeL = (g_dwOtherModeL&(~dwMask)) | dwData;

	BOOL bZCompare		= (g_dwOtherModeL & Z_CMP)			? TRUE : FALSE;
	BOOL bZUpdate		= (g_dwOtherModeL & Z_UPD)			? TRUE : FALSE;
	
	g_Renderer->SetZCompare( bZCompare );
	g_Renderer->SetZUpdate( bZUpdate );
	

	//FIXME
	BOOL bZModeDecal    = (g_dwOtherModeL & ZMODE_DEC) == ZMODE_DEC     ? TRUE : FALSE;
	g_Renderer->SetZmodeDecal(bZModeDecal);

	u32 dwAlphaTestMode = (g_dwOtherModeL >> G_MDSFT_ALPHACOMPARE) & 0x3;

	//#define	G_AC_NONE			(0 << G_MDSFT_ALPHACOMPARE)
	//#define	G_AC_THRESHOLD		(1 << G_MDSFT_ALPHACOMPARE)
	//#define	G_AC_DITHER			(3 << G_MDSFT_ALPHACOMPARE)		
}



void RDP_GFX_SetOtherMode_H(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwShift = (dwCmd0>>8)&0xFF;
	u32 dwLength= (dwCmd0   )&0xFF;
	u32 dwData  = dwCmd1;

	u32 dwMask = ((1<<dwLength)-1)<<dwShift;
	u32 dwOldSettings = g_dwOtherModeH;

	// Mask off bits that don't apply. Might not be necessary
//	dwData &= dwMask;

	g_dwOtherModeH = (g_dwOtherModeH&(~dwMask)) | dwData;


	BOOL bTexPersp    = (g_dwOtherModeH & G_TP_PERSP) ? TRUE : FALSE;
	u32 dwCycleType = (g_dwOtherModeH>>G_MDSFT_CYCLETYPE)&0x3;
	u32 dwTextFilt  = (g_dwOtherModeH>>G_MDSFT_TEXTFILT)&0x3;
	
	g_Renderer->SetTexturePerspective( bTexPersp );
	g_Renderer->SetCycleType( dwCycleType );
	

	switch (dwTextFilt<<G_MDSFT_TEXTFILT)
	{
		case G_TF_POINT:
			g_Renderer->SetFilter(0, PVR_FILTER_NONE);
			break;
		case G_TF_AVERAGE:	// ?
		    g_Renderer->SetFilter(0, PVR_FILTER_BILINEAR);
			break;
		case G_TF_BILERP:
            g_Renderer->SetFilter(0, PVR_FILTER_BILINEAR);
			break;
		default:
			// Unknown!
            g_Renderer->SetFilter(0, PVR_FILTER_NONE);
			break;
	}
}

void RDP_GFX_ZeldaSetOtherMode_H(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwShift = (dwCmd0>>8)&0xFF;
	u32 dwLength= (dwCmd0   )&0xFF;
	u32 dwData  = dwCmd1;

	// Mask is constructed slightly differently
	u32 dwMask = (u32)((s32)(0x80000000)>>dwLength)>>dwShift;
	u32 dwMask2 = ((1<<dwLength)-1)<<dwShift;

	u32 dwOldSettings = g_dwOtherModeH;

	// Mask off bits that don't apply. Might not be necessary
//	dwData &= dwMask;

	g_dwOtherModeH = (g_dwOtherModeH&(~dwMask)) | dwData;

	// These don't seem to work properly in ucode 5!!
	g_dwOtherModeH &= ~(3<<G_MDSFT_CYCLETYPE);
	g_dwOtherModeH |= G_CYC_1CYCLE<<G_MDSFT_CYCLETYPE;
 
	BOOL bTexPersp    = (g_dwOtherModeH & G_TP_PERSP) ? TRUE : FALSE;
	u32 dwCycleType = (g_dwOtherModeH>>G_MDSFT_CYCLETYPE)&0x3;
	u32 dwTextFilt  = (g_dwOtherModeH>>G_MDSFT_TEXTFILT)&0x3;

	g_Renderer->SetTexturePerspective( TRUE ); // bTexPersp );
	g_Renderer->SetCycleType( dwCycleType );
	

	switch (dwTextFilt<<G_MDSFT_TEXTFILT)
	{
		case G_TF_POINT:
			g_Renderer->SetFilter(0, PVR_FILTER_NONE);
			break;
		case G_TF_AVERAGE:	// ?
		    g_Renderer->SetFilter(0, PVR_FILTER_BILINEAR);
			break;
		case G_TF_BILERP:
            g_Renderer->SetFilter(0, PVR_FILTER_BILINEAR);
			break;
		default:
			// Unknown!
            g_Renderer->SetFilter(0, PVR_FILTER_NONE);
			break;
	}
}

static void RDP_GFX_InitGeometryMode()
{
	BOOL bCullFront		= (g_dwGeometryMode & G_CULL_FRONT) ? TRUE : FALSE;
	BOOL bCullBack		= (g_dwGeometryMode & G_CULL_BACK) ? TRUE : FALSE;
	
	BOOL bShade			= (g_dwGeometryMode & G_SHADE) ? TRUE : FALSE;
	BOOL bShadeSmooth	= (g_dwGeometryMode & G_SHADING_SMOOTH) ? TRUE : FALSE;
	
	BOOL bFog			= (g_dwGeometryMode & G_FOG) ? TRUE : FALSE;
	BOOL bTextureGen	= (g_dwGeometryMode & G_TEXTURE_GEN) ? TRUE : FALSE;

	BOOL bLighting      = (g_dwGeometryMode & G_LIGHTING) ? TRUE : FALSE;
	BOOL bZBuffer		= (g_dwGeometryMode & G_ZBUFFER)	? TRUE : FALSE;	

	g_Renderer->SetCullMode(bCullFront, bCullBack);
	
	/*KOS
	if (bShade && bShadeSmooth)		g_Renderer->SetShadeMode( D3DSHADE_GOURAUD );
	else							g_Renderer->SetShadeMode( D3DSHADE_FLAT );
	*/
	
	g_Renderer->SetFogEnable( bFog );
	g_Renderer->SetTextureGen(bTextureGen);

	g_Renderer->SetLighting( bLighting );
	g_Renderer->ZBufferEnable( bZBuffer );
}

void RDP_GFX_ClearGeometryMode(u32 dwCmd0, u32 dwCmd1)
{

	u32 dwMask = (dwCmd1);
	g_dwGeometryMode &= ~dwMask;


	//g_bTextureEnable = (g_dwGeometryMode & G_TEXTURE_ENABLE);

	RDP_GFX_InitGeometryMode();
}



void RDP_GFX_SetGeometryMode(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwMask = (dwCmd1);
	g_dwGeometryMode |= dwMask;

	//g_bTextureEnable = (g_dwGeometryMode & G_TEXTURE_ENABLE);

	RDP_GFX_InitGeometryMode();
}

// NOT CHECKED
/*
#define G_ZBUFFER				0x00000001
#define G_TEXTURE_ENABLE		0x00000002	// Microcode use only 
#define G_SHADE					0x00000004	// enable Gouraud interp
#define G_SHADING_SMOOTH		0x00000200	// flat or smooth shaded
#define G_CULL_FRONT			0x00001000
#define G_CULL_BACK				0x00002000
#define G_CULL_BOTH				0x00003000	// To make code cleaner
#define G_FOG					0x00010000
#define G_LIGHTING				0x00020000
#define G_TEXTURE_GEN			0x00040000
#define G_TEXTURE_GEN_LINEAR	0x00080000*/

#define G_ZELDA_ZBUFFER			0x00000001		// Guess
#define G_ZELDA_CULL_BACK		0x00000200
#define G_ZELDA_CULL_FRONT		0x00000400
#define G_ZELDA_FOG				0x00010000
#define G_ZELDA_LIGHTING		0x00020000
#define G_ZELDA_TEXTURE_GEN		0x00040000
#define G_ZELDA_SHADING_FLAT	0x00080000
//#define G_ZELDA_SHADE			0x00080000

/*
00000000 00101110 00000100 00000101    002E0405  texgen(0) flat(1) cull(2) light(1)
00000000 00100011 00000100 00000101    00230405  texgen(0) flat(0) cull(2) light(1)
           n FFL  cccccccc xxxxxxxL?
00000000 00000000 00000000 00000000
*mode geometry: 002E0405  texgen(0) flat(1) cull(2) light(1)
*mode geometry: 00230405  texgen(0) flat(0) cull(2) light(1)
*mode geometry: 00220405  texgen(0) flat(0) cull(2) light(1)
*mode geometry: 00220405  texgen(0) flat(0) cull(2) light(1)
*mode geometry: 00220405  texgen(0) flat(0) cull(2) light(1)
*mode geometry: 00200404  texgen(0) flat(0) cull(2) light(0)
*mode geometry: 00200204  texgen(0) flat(0) cull(1) light(0)
*mode geometry: 00200000  texgen(0) flat(0) cull(0) light(0)
*mode geometry: 00200000  texgen(0) flat(0) cull(0) light(0)
*mode geometry: 00000000  texgen(0) flat(0) cull(0) light(0)
*/

// Seems to be AND (dwCmd0&0xFFFFFF) OR (dwCmd1&0xFFFFFF)
void RDP_GFX_ZeldaSetGeometryMode(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwAnd = (dwCmd0) & 0x00FFFFFF;
	u32 dwOr  = (dwCmd1) & 0x00FFFFFF;

	g_dwGeometryMode &= dwAnd;
	g_dwGeometryMode |= dwOr;


	BOOL bCullFront		= (g_dwGeometryMode & G_ZELDA_CULL_FRONT) ? TRUE : FALSE;
	BOOL bCullBack		= (g_dwGeometryMode & G_ZELDA_CULL_BACK) ? TRUE : FALSE;
	
	//BOOL bShade			= (g_dwGeometryMode & G_ZELDA_SHADE) ? TRUE : FALSE;
	BOOL bFlatShade		= (g_dwGeometryMode & G_ZELDA_SHADING_FLAT) ? TRUE : FALSE;
	
	BOOL bFog			= (g_dwGeometryMode & G_ZELDA_FOG) ? TRUE : FALSE;
	BOOL bTextureGen	= (g_dwGeometryMode & G_ZELDA_TEXTURE_GEN) ? TRUE : FALSE;

	BOOL bLighting      = (g_dwGeometryMode & G_ZELDA_LIGHTING) ? TRUE : FALSE;
	BOOL bZBuffer		= (g_dwGeometryMode & G_ZELDA_ZBUFFER)	? TRUE : FALSE;	

	g_Renderer->SetCullMode(bCullFront, bCullBack);
	
	/*KOS
	if (bFlatShade)	g_Renderer->SetShadeMode( D3DSHADE_FLAT );
	else			g_Renderer->SetShadeMode( D3DSHADE_GOURAUD );
	*/
	
	g_Renderer->SetFogEnable( bFog );
	g_Renderer->SetTextureGen(bTextureGen);

	g_Renderer->SetLighting( bLighting );
	g_Renderer->ZBufferEnable( bZBuffer );

	//RDP_GFX_InitGeometryMode();
}



void RDP_GFX_SetCombine(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwMux0 = dwCmd0&0x00FFFFFF;
	u32 dwMux1 = dwCmd1;

	
	g_Renderer->SetMux(dwMux0, dwMux1);
}

void RDP_GFX_SetFillColor(u32 dwCmd0, u32 dwCmd1)
{
	// TODO - Check colour image format to work out how this ahould be decoded!
	g_dwFillColor = Convert555ToRGBA((WORD)dwCmd1);

	DL_PF("    Color5551=0x%04x = 0x%08x", (WORD)dwCmd1, g_dwFillColor);

}

void RDP_GFX_SetFogColor(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwRed		= (dwCmd1>>24)&0xFF;
	u32 dwGreen	= (dwCmd1>>16)&0xFF;
	u32 dwBlue	= (dwCmd1>>8)&0xFF;
	u32 dwAlpha	= (dwCmd1)&0xFF;

	DL_PF("    RGBA: %d %d %d %d", dwRed, dwGreen, dwBlue, dwAlpha);

	u32 dwFogColor = dwCmd1;

	g_Renderer->SetFogColor( RGBA_MAKE(dwRed, dwGreen, dwBlue, dwAlpha) );
}

void RDP_GFX_SetBlendColor(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwRed		= (dwCmd1>>24)&0xFF;
	u32 dwGreen	= (dwCmd1>>16)&0xFF;
	u32 dwBlue	= (dwCmd1>>8)&0xFF;
	u32 dwAlpha	= (dwCmd1)&0xFF;

	DL_PF("    RGBA: %d %d %d %d", dwRed, dwGreen, dwBlue, dwAlpha);

	g_Renderer->SetAlphaRef(dwAlpha);
}


void RDP_GFX_SetPrimColor(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwM		= (dwCmd0>>8)&0xFF;
	u32 dwL		= (dwCmd0)&0xFF;
	u32 dwRed		= (dwCmd1>>24)&0xFF;
	u32 dwGreen	= (dwCmd1>>16)&0xFF;
	u32 dwBlue	= (dwCmd1>>8)&0xFF;
	u32 dwAlpha	= (dwCmd1)&0xFF;

	DL_PF("    M:%d L:%d RGBA: %d %d %d %d", 
		dwM, dwL, dwRed, dwGreen, dwBlue, dwAlpha);

	// Goldeneye moving-circles hack
	// Originally this did mess something else up - not sure what now!
	// Mux0=0x00ffffff Mux1=0xfffdf6fb
//	if (dwAlpha == 0)
//		dwAlpha = 255;

	// Set TFACTOR
	g_Renderer->SetPrimitiveColor( RGBA_MAKE(dwRed, dwGreen, dwBlue, dwAlpha));

}

void RDP_GFX_SetEnvColor(u32 dwCmd0, u32 dwCmd1)
{
	u32 dwRed		= (dwCmd1>>24)&0xFF;
	u32 dwGreen	= (dwCmd1>>16)&0xFF;
	u32 dwBlue	= (dwCmd1>>8)&0xFF;
	u32 dwAlpha	= (dwCmd1)&0xFF;
	DL_PF("    RGBA: %d %d %d %d",
		dwRed, dwGreen, dwBlue, dwAlpha);

	g_Renderer->SetEnvColor( RGBA_MAKE(dwRed, dwGreen, dwBlue, dwAlpha));
}

