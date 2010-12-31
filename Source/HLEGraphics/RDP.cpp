/*

  Copyright (C) 2002 StrmnNrmn

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
#include "RDP.h"

#include "DLParser.h"

#include "DebugDisplayList.h"
#include "Debug/DBGConsole.h"

#include "OSHLE/ultra_gbi.h"
#include "Core/Memory.h"


extern SImageDescriptor g_TI;

//*****************************************************************************
// RDP state
//*****************************************************************************
RDP_OtherMode		gRDPOtherMode;
RDP_Mux				gRDPMux;
RDP_Tile			gRDPTiles[8];
RDP_TileSize		gRDPTileSizes[8];

//u8		gTextureMemory[ 4096 ];
u8 *gTextureMemory;



//*************************************************************************************
// Set the RDP tile
//*************************************************************************************
void	RDP_SetTile( RDP_Tile tile )
{
	gRDPTiles[ tile.tile_idx ] = tile;
}

//*************************************************************************************
// Set the RDP tile size
//*************************************************************************************
void	RDP_SetTileSize( RDP_TileSize tile_size )
{
	gRDPTileSizes[ tile_size.tile_idx ] = tile_size;
}
//*****************************************************************************
//
//*****************************************************************************
void	RDP_SetMux( u64 mux )
{
	gRDPMux._u64 = mux;
}

//*****************************************************************************
//
//*****************************************************************************
// ToDO : Optimize RDP_LoadBlock, it causes a big impact to emulation (as expected, but we should be able to make it faster) 
// Also try to improve it since sometimes it doesn't work as expected.
// Good test case is Majora's Mask/
//
void RDP_LoadBlock( RDP_TileSize command )
{
#if RDP_EMULATE_TMEM
	//u32 dwULS		= command.left / 4;		// 0
	//u32 dwULT		= command.top  / 4;		// 0
	u32 dwTile		= command.tile_idx;
	u32 dwPixels	= command.right + 1;	// Number of bytes-1?
	u32 dwDXT		= command.bottom;		// 1.11 fixed point

	u32 dwQWs;
	u32 dwBytesPerLine;

	if (dwDXT == 0)
	{
		dwQWs = 1;
		dwBytesPerLine = 8;
	}
	else
	{
		dwQWs = 2048 / dwDXT;						// #Quad Words
		dwBytesPerLine = (2048 * 8) / dwDXT;
		dwBytesPerLine = (dwBytesPerLine + 7 ) & ~7;
	}

	u32 total_bytes = ((dwPixels<< g_TI.Size)+1)/2;

//	DBGConsole_Msg( 0, "    Tile:%d (%d,%d) %d pixels DXT:0x%04x = %d QWs => %d pixels/line", dwTile, dwULS, dwULT, dwPixels, dwDXT, dwQWs, dwPitch);
//	DBGConsole_Msg( 0, "    Offset: 0x%08x", dwSrcOffset );

//	gLoadAddresses[ gRDPTiles[command.tile_idx].tile.tmem & 0xfff ] = dwSrcOffset;
//	gLoadPitches[ gRDPTiles[command.tile_idx].tile.tmem & 0xfff ] = ~0;

	u32 ram = g_TI.Address;
	u32 tmem = gRDPTiles[ dwTile ].tmem << 3;		// Quads

	if ( dwDXT == 0 )
	{
		for ( u32 i = 0; i < total_bytes; i++ )
		{
			gTextureMemory[ (tmem+i) ] = g_pu8RamBase[ (ram+i) ];
		}
	}
	else
	{
		for ( u32 bytes = 0, row = 0; bytes < total_bytes; bytes += dwBytesPerLine, row++ )
		{
			if ( (row&1) == 0 )
			{
				for ( u32 i = 0; i < dwBytesPerLine; i++ )
				{
					gTextureMemory[ (tmem+i) ] = g_pu8RamBase[ (ram+i) ];
				}
			}
			else
			{
				for ( u32 i = 0; i < dwBytesPerLine; i++ )
				{
					gTextureMemory[ (tmem+i) ^ 0x4 ] = g_pu8RamBase[ (ram+i) ];
				}
			}

			tmem += dwBytesPerLine;
			ram += dwBytesPerLine;
		}
	}
#endif
}

//*************************************************************************************
// Load a tile into texture memory
//*************************************************************************************
void	RDP_LoadTile( RDP_TileSize tile_size  )
{
#if RDP_EMULATE_TMEM
	u32 left	= tile_size.left/4;
	u32 top		= tile_size.top/4;
	u32 dwTile	= tile_size.tile_idx;
	u32 right	= tile_size.right/4;
	u32 bottom	= tile_size.bottom/4;


#define SIZ_TO_BYTES( x, siz )		(((x)<<(siz))+1)/2

	// Todo - check if this is odd, and round up?
	u32 pitch = SIZ_TO_BYTES( g_TI.Width, g_TI.Size );
	u32 dwOffset = ( pitch * top ) + SIZ_TO_BYTES( left, g_TI.Size );

//	DBGConsole_Msg(0, "    Tile:%d (%d,%d) -> (%d,%d) [%d x %d]",			dwTile, left, top, right, bottom, (right - left)+1, (bottom - top)+1);
//	DBGConsole_Msg(0, "    g_TI.Address + dwOffset: 0x%08x (LoadTile Pitch: %d)",		dwSrcOffset, pitch);

//	gLoadAddresses[ g_Tiles[dwTile].tile.tmem & 0xfff ] = g_TI.Address;
//	gLoadPitches[ g_Tiles[dwTile].tile.tmem & 0xfff ] = dwPitch;

	u32 ram = g_TI.Address + dwOffset;
	u32 tmem = gRDPTiles[ dwTile ].tmem << 3;		// Quads

	u32 width = right + 1 - left;

	u32 bytes_per_line = SIZ_TO_BYTES( width, g_TI.Size );

	for ( u32 y = top; y <= bottom; y++ )
	{
		if ( ((y-top)&1) == 0 )
		{
			for ( u32 i = 0; i < bytes_per_line; i++ )
			{
				DAEDALUS_DL_ASSERT( tmem+i < 0xfff, "Out of range for tmem" );
				gTextureMemory[ (tmem+i) ] = g_pu8RamBase[ (ram+i) ];
			}
		}
		else
		{
			for ( u32 i = 0; i < bytes_per_line; i++ )
			{
				DAEDALUS_DL_ASSERT( ((tmem+i)^0x4) < 0xfff, "Out of range for tmem" );
				gTextureMemory[ (tmem+i)^0x4 ] = g_pu8RamBase[ (ram+i) ];
			}
		}

		tmem = ( tmem + bytes_per_line + 7 ) & ~7;
		ram += pitch;
	}
#endif
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*****************************************************************************
//
//*****************************************************************************

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

static const char * sc_szBlClr[4] = { "In",  "Mem",  "Bl",     "Fog" };
static const char * sc_szBlA1[4]  = { "AIn", "AFog", "AShade", "0" };
static const char * sc_szBlA2[4]  = { "1-A", "AMem", "1",      "?" };
/*
#define	GBL_c1(m1a, m1b, m2a, m2b)	\
	(m1a) << 30 | (m1b) << 26 | (m2a) << 22 | (m2b) << 18
#define	GBL_c2(m1a, m1b, m2a, m2b)	\
	(m1a) << 28 | (m1b) << 24 | (m2a) << 20 | (m2b) << 16
*/

/*
  Blend: 0x0050: In*AIn + Mem*1-A    | In*AIn + Mem*1-A				// XLU    | XLU
  Blend: 0x0055: In*AIn + Mem*AMem   | In*AIn + Mem*AMem			// OPA    | OPA
  Blend: 0x0f0a: In*0 + In*1         | In*0 + In*1					// PASS   | PASS

  Blend: 0x0150: In*AIn + Mem*1-A    | In*AFog+ Mem*1-A				// XLU    |
  Blend: 0x0c18: In*0 + In*1         | In*AIn + Mem*1-A				// PASS   | XLU
  Blend: 0x0c19: In*0 + In*1         | In*AIn + Mem*AMem 			// PASS   | OPA
  Blend: 0xc810: Fog*AShade + In*1-A | In*AIn + Mem*1-A  			// FOG_SH | XLU
  Blend: 0xc811: Fog*AShade + In*1-A | In*AIn + Mem*AMem 			// FOG_SH | OPA
*/
/*

#define	G_RM_FOG_SHADE_A	GBL_c1(G_BL_CLR_FOG,	G_BL_A_SHADE,	G_BL_CLR_IN, G_BL_1MA)
#define	G_RM_FOG_PRIM_A		GBL_c1(G_BL_CLR_FOG,	G_BL_A_FOG,		G_BL_CLR_IN, G_BL_1MA)
#define	G_RM_PASS			GBL_c1(G_BL_CLR_IN,		G_BL_0,			G_BL_CLR_IN, G_BL_1)
*/


#define	G_BL_CLR_IN	0
#define	G_BL_CLR_MEM	1
#define	G_BL_CLR_BL	2
#define	G_BL_CLR_FOG	3
#define	G_BL_1MA	0
#define	G_BL_A_MEM	1
#define	G_BL_A_IN	0
#define	G_BL_A_FOG	1
#define	G_BL_A_SHADE	2
#define	G_BL_1		2
#define	G_BL_0		3

#define	GBL_c1(m1a, m1b, m2a, m2b)	\
	(m1a) << 30 | (m1b) << 26 | (m2a) << 22 | (m2b) << 18
#define	GBL_c2(m1a, m1b, m2a, m2b)	\
	(m1a) << 28 | (m1b) << 24 | (m2a) << 20 | (m2b) << 16

#define	RM_AA_ZB_OPA_SURF(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_RA_ZB_OPA_SURF(clk)					\
	AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP |			\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_ZB_XLU_SURF(clk)					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |	\
	FORCE_BL | ZMODE_XLU |					\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_OPA_DECAL(clk)					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | ALPHA_CVG_SEL |	\
	ZMODE_DEC |						\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_RA_ZB_OPA_DECAL(clk)					\
	AA_EN | Z_CMP | CVG_DST_WRAP | ALPHA_CVG_SEL |		\
	ZMODE_DEC |						\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_ZB_XLU_DECAL(clk)					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |	\
	FORCE_BL | ZMODE_DEC |					\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_OPA_INTER(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	ALPHA_CVG_SEL |	ZMODE_INTER |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_RA_ZB_OPA_INTER(clk)					\
	AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP |			\
	ALPHA_CVG_SEL |	ZMODE_INTER |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_ZB_XLU_INTER(clk)					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |	\
	FORCE_BL | ZMODE_INTER |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_XLU_LINE(clk)					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_CLAMP | CVG_X_ALPHA |	\
	ALPHA_CVG_SEL | FORCE_BL | ZMODE_XLU |			\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_DEC_LINE(clk)					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_SAVE | CVG_X_ALPHA |	\
	ALPHA_CVG_SEL | FORCE_BL | ZMODE_DEC |			\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_TEX_EDGE(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_ZB_TEX_INTER(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_INTER | TEX_EDGE |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_ZB_SUB_SURF(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_FULL |		\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_ZB_PCL_SURF(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	ZMODE_OPA | G_AC_DITHER | 				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_OPA_TERR(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_TEX_TERR(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_SUB_TERR(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_FULL |		\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)


#define	RM_AA_OPA_SURF(clk)					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_RA_OPA_SURF(clk)					\
	AA_EN | CVG_DST_CLAMP |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_XLU_SURF(clk)					\
	AA_EN | IM_RD | CVG_DST_WRAP | CLR_ON_CVG | FORCE_BL |	\
	ZMODE_OPA |						\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_XLU_LINE(clk)					\
	AA_EN | IM_RD | CVG_DST_CLAMP | CVG_X_ALPHA |		\
	ALPHA_CVG_SEL | FORCE_BL | ZMODE_OPA |			\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_DEC_LINE(clk)					\
	AA_EN | IM_RD | CVG_DST_FULL | CVG_X_ALPHA |		\
	ALPHA_CVG_SEL | FORCE_BL | ZMODE_OPA |			\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_TEX_EDGE(clk)					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_SUB_SURF(clk)					\
	AA_EN | IM_RD | CVG_DST_FULL |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_PCL_SURF(clk)					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	ZMODE_OPA | G_AC_DITHER | 				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_OPA_TERR(clk)					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_TEX_TERR(clk)					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_SUB_TERR(clk)					\
	AA_EN | IM_RD | CVG_DST_FULL |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)


#define	RM_ZB_OPA_SURF(clk)					\
	Z_CMP | Z_UPD | CVG_DST_FULL | ALPHA_CVG_SEL |		\
	ZMODE_OPA |						\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_ZB_XLU_SURF(clk)					\
	Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_XLU |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_ZB_OPA_DECAL(clk)					\
	Z_CMP | CVG_DST_FULL | ALPHA_CVG_SEL | ZMODE_DEC |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_ZB_XLU_DECAL(clk)					\
	Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_DEC |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_ZB_CLD_SURF(clk)					\
	Z_CMP | IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_XLU |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_ZB_OVL_SURF(clk)					\
	Z_CMP | IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_DEC |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_ZB_PCL_SURF(clk)					\
	Z_CMP | Z_UPD | CVG_DST_FULL | ZMODE_OPA |		\
	G_AC_DITHER | 						\
	GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)


#define	RM_OPA_SURF(clk)					\
	CVG_DST_CLAMP | FORCE_BL | ZMODE_OPA |			\
	GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define	RM_XLU_SURF(clk)					\
	IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_OPA |		\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_TEX_EDGE(clk)					\
	CVG_DST_CLAMP | CVG_X_ALPHA | ALPHA_CVG_SEL | FORCE_BL |\
	ZMODE_OPA | TEX_EDGE | AA_EN |					\
	GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define	RM_CLD_SURF(clk)					\
	IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_OPA |		\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_PCL_SURF(clk)					\
	CVG_DST_FULL | FORCE_BL | ZMODE_OPA | 			\
	G_AC_DITHER | 						\
	GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define	RM_ADD(clk)					\
	IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_OPA |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_FOG, G_BL_CLR_MEM, G_BL_1)

#define	RM_NOOP(clk)	\
	GBL_c##clk(0, 0, 0, 0)

#define RM_VISCVG(clk) \
	IM_RD | FORCE_BL |     \
	GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_BL, G_BL_A_MEM)

/* for rendering to an 8-bit framebuffer */
#define RM_OPA_CI(clk)                    \
	CVG_DST_CLAMP | ZMODE_OPA |          \
	GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)



#define	G_RM_AA_ZB_OPA_SURF	RM_AA_ZB_OPA_SURF(1)
#define	G_RM_AA_ZB_OPA_SURF2	RM_AA_ZB_OPA_SURF(2)
#define	G_RM_AA_ZB_XLU_SURF	RM_AA_ZB_XLU_SURF(1)
#define	G_RM_AA_ZB_XLU_SURF2	RM_AA_ZB_XLU_SURF(2)
#define	G_RM_AA_ZB_OPA_DECAL	RM_AA_ZB_OPA_DECAL(1)
#define	G_RM_AA_ZB_OPA_DECAL2	RM_AA_ZB_OPA_DECAL(2)
#define	G_RM_AA_ZB_XLU_DECAL	RM_AA_ZB_XLU_DECAL(1)
#define	G_RM_AA_ZB_XLU_DECAL2	RM_AA_ZB_XLU_DECAL(2)
#define	G_RM_AA_ZB_OPA_INTER	RM_AA_ZB_OPA_INTER(1)
#define	G_RM_AA_ZB_OPA_INTER2	RM_AA_ZB_OPA_INTER(2)
#define	G_RM_AA_ZB_XLU_INTER	RM_AA_ZB_XLU_INTER(1)
#define	G_RM_AA_ZB_XLU_INTER2	RM_AA_ZB_XLU_INTER(2)
#define	G_RM_AA_ZB_XLU_LINE	RM_AA_ZB_XLU_LINE(1)
#define	G_RM_AA_ZB_XLU_LINE2	RM_AA_ZB_XLU_LINE(2)
#define	G_RM_AA_ZB_DEC_LINE	RM_AA_ZB_DEC_LINE(1)
#define	G_RM_AA_ZB_DEC_LINE2	RM_AA_ZB_DEC_LINE(2)
#define	G_RM_AA_ZB_TEX_EDGE	RM_AA_ZB_TEX_EDGE(1)
#define	G_RM_AA_ZB_TEX_EDGE2	RM_AA_ZB_TEX_EDGE(2)
#define	G_RM_AA_ZB_TEX_INTER	RM_AA_ZB_TEX_INTER(1)
#define	G_RM_AA_ZB_TEX_INTER2	RM_AA_ZB_TEX_INTER(2)
#define	G_RM_AA_ZB_SUB_SURF	RM_AA_ZB_SUB_SURF(1)
#define	G_RM_AA_ZB_SUB_SURF2	RM_AA_ZB_SUB_SURF(2)
#define	G_RM_AA_ZB_PCL_SURF	RM_AA_ZB_PCL_SURF(1)
#define	G_RM_AA_ZB_PCL_SURF2	RM_AA_ZB_PCL_SURF(2)
#define	G_RM_AA_ZB_OPA_TERR	RM_AA_ZB_OPA_TERR(1)
#define	G_RM_AA_ZB_OPA_TERR2	RM_AA_ZB_OPA_TERR(2)
#define	G_RM_AA_ZB_TEX_TERR	RM_AA_ZB_TEX_TERR(1)
#define	G_RM_AA_ZB_TEX_TERR2	RM_AA_ZB_TEX_TERR(2)
#define	G_RM_AA_ZB_SUB_TERR	RM_AA_ZB_SUB_TERR(1)
#define	G_RM_AA_ZB_SUB_TERR2	RM_AA_ZB_SUB_TERR(2)

#define	G_RM_RA_ZB_OPA_SURF	RM_RA_ZB_OPA_SURF(1)
#define	G_RM_RA_ZB_OPA_SURF2	RM_RA_ZB_OPA_SURF(2)
#define	G_RM_RA_ZB_OPA_DECAL	RM_RA_ZB_OPA_DECAL(1)
#define	G_RM_RA_ZB_OPA_DECAL2	RM_RA_ZB_OPA_DECAL(2)
#define	G_RM_RA_ZB_OPA_INTER	RM_RA_ZB_OPA_INTER(1)
#define	G_RM_RA_ZB_OPA_INTER2	RM_RA_ZB_OPA_INTER(2)

#define	G_RM_AA_OPA_SURF	RM_AA_OPA_SURF(1)
#define	G_RM_AA_OPA_SURF2	RM_AA_OPA_SURF(2)
#define	G_RM_AA_XLU_SURF	RM_AA_XLU_SURF(1)
#define	G_RM_AA_XLU_SURF2	RM_AA_XLU_SURF(2)
#define	G_RM_AA_XLU_LINE	RM_AA_XLU_LINE(1)
#define	G_RM_AA_XLU_LINE2	RM_AA_XLU_LINE(2)
#define	G_RM_AA_DEC_LINE	RM_AA_DEC_LINE(1)
#define	G_RM_AA_DEC_LINE2	RM_AA_DEC_LINE(2)
#define	G_RM_AA_TEX_EDGE	RM_AA_TEX_EDGE(1)
#define	G_RM_AA_TEX_EDGE2	RM_AA_TEX_EDGE(2)
#define	G_RM_AA_SUB_SURF	RM_AA_SUB_SURF(1)
#define	G_RM_AA_SUB_SURF2	RM_AA_SUB_SURF(2)
#define	G_RM_AA_PCL_SURF	RM_AA_PCL_SURF(1)
#define	G_RM_AA_PCL_SURF2	RM_AA_PCL_SURF(2)
#define	G_RM_AA_OPA_TERR	RM_AA_OPA_TERR(1)
#define	G_RM_AA_OPA_TERR2	RM_AA_OPA_TERR(2)
#define	G_RM_AA_TEX_TERR	RM_AA_TEX_TERR(1)
#define	G_RM_AA_TEX_TERR2	RM_AA_TEX_TERR(2)
#define	G_RM_AA_SUB_TERR	RM_AA_SUB_TERR(1)
#define	G_RM_AA_SUB_TERR2	RM_AA_SUB_TERR(2)

#define	G_RM_RA_OPA_SURF	RM_RA_OPA_SURF(1)
#define	G_RM_RA_OPA_SURF2	RM_RA_OPA_SURF(2)

#define	G_RM_ZB_OPA_SURF	RM_ZB_OPA_SURF(1)
#define	G_RM_ZB_OPA_SURF2	RM_ZB_OPA_SURF(2)
#define	G_RM_ZB_XLU_SURF	RM_ZB_XLU_SURF(1)
#define	G_RM_ZB_XLU_SURF2	RM_ZB_XLU_SURF(2)
#define	G_RM_ZB_OPA_DECAL	RM_ZB_OPA_DECAL(1)
#define	G_RM_ZB_OPA_DECAL2	RM_ZB_OPA_DECAL(2)
#define	G_RM_ZB_XLU_DECAL	RM_ZB_XLU_DECAL(1)
#define	G_RM_ZB_XLU_DECAL2	RM_ZB_XLU_DECAL(2)
#define	G_RM_ZB_CLD_SURF	RM_ZB_CLD_SURF(1)
#define	G_RM_ZB_CLD_SURF2	RM_ZB_CLD_SURF(2)
#define	G_RM_ZB_OVL_SURF	RM_ZB_OVL_SURF(1)
#define	G_RM_ZB_OVL_SURF2	RM_ZB_OVL_SURF(2)
#define	G_RM_ZB_PCL_SURF	RM_ZB_PCL_SURF(1)
#define	G_RM_ZB_PCL_SURF2	RM_ZB_PCL_SURF(2)

#define	G_RM_OPA_SURF		RM_OPA_SURF(1)
#define	G_RM_OPA_SURF2		RM_OPA_SURF(2)
#define	G_RM_XLU_SURF		RM_XLU_SURF(1)
#define	G_RM_XLU_SURF2		RM_XLU_SURF(2)
#define	G_RM_CLD_SURF		RM_CLD_SURF(1)
#define	G_RM_CLD_SURF2		RM_CLD_SURF(2)
#define	G_RM_TEX_EDGE		RM_TEX_EDGE(1)
#define	G_RM_TEX_EDGE2		RM_TEX_EDGE(2)
#define	G_RM_PCL_SURF		RM_PCL_SURF(1)
#define	G_RM_PCL_SURF2		RM_PCL_SURF(2)
#define G_RM_ADD       		RM_ADD(1)
#define G_RM_ADD2      		RM_ADD(2)
#define G_RM_NOOP       	RM_NOOP(1)
#define G_RM_NOOP2      	RM_NOOP(2)
#define G_RM_VISCVG    		RM_VISCVG(1)
#define G_RM_VISCVG2    	RM_VISCVG(2)
#define G_RM_OPA_CI         RM_OPA_CI(1)
#define G_RM_OPA_CI2        RM_OPA_CI(2)


#define	G_RM_FOG_SHADE_A	GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA)
#define	G_RM_FOG_PRIM_A		GBL_c1(G_BL_CLR_FOG, G_BL_A_FOG, G_BL_CLR_IN, G_BL_1MA)
#define	G_RM_PASS		GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)


const char *	GetBlenderModeDescription( u32 mode )
{
	switch ( mode & ~3 )
	{
		case 	G_RM_AA_ZB_OPA_SURF:			return "G_RM_AA_ZB_OPA_SURF";
		case 	G_RM_AA_ZB_OPA_SURF2:			return "G_RM_AA_ZB_OPA_SURF2";
		case 	G_RM_AA_ZB_XLU_SURF:			return "G_RM_AA_ZB_XLU_SURF";
		case 	G_RM_AA_ZB_XLU_SURF2:			return "G_RM_AA_ZB_XLU_SURF2";
		case 	G_RM_AA_ZB_OPA_DECAL:			return "G_RM_AA_ZB_OPA_DECAL";
		case 	G_RM_AA_ZB_OPA_DECAL2:			return "G_RM_AA_ZB_OPA_DECAL2";
		case 	G_RM_AA_ZB_XLU_DECAL:			return "G_RM_AA_ZB_XLU_DECAL";
		case 	G_RM_AA_ZB_XLU_DECAL2:			return "G_RM_AA_ZB_XLU_DECAL2";
		case 	G_RM_AA_ZB_OPA_INTER:			return "G_RM_AA_ZB_OPA_INTER";
		case 	G_RM_AA_ZB_OPA_INTER2:			return "G_RM_AA_ZB_OPA_INTER2";
		case 	G_RM_AA_ZB_XLU_INTER:			return "G_RM_AA_ZB_XLU_INTER";
		case 	G_RM_AA_ZB_XLU_INTER2:			return "G_RM_AA_ZB_XLU_INTER2";
		case 	G_RM_AA_ZB_XLU_LINE:			return "G_RM_AA_ZB_XLU_LINE";
		case 	G_RM_AA_ZB_XLU_LINE2:			return "G_RM_AA_ZB_XLU_LINE2";
		case 	G_RM_AA_ZB_DEC_LINE:			return "G_RM_AA_ZB_DEC_LINE";
		case 	G_RM_AA_ZB_DEC_LINE2:			return "G_RM_AA_ZB_DEC_LINE2";
		case 	G_RM_AA_ZB_TEX_EDGE:			return "G_RM_AA_ZB_TEX_EDGE";
		case 	G_RM_AA_ZB_TEX_EDGE2:			return "G_RM_AA_ZB_TEX_EDGE2";
		case 	G_RM_AA_ZB_TEX_INTER:			return "G_RM_AA_ZB_TEX_INTER";
		case 	G_RM_AA_ZB_TEX_INTER2:			return "G_RM_AA_ZB_TEX_INTER2";
		case 	G_RM_AA_ZB_SUB_SURF:			return "G_RM_AA_ZB_SUB_SURF";
		case 	G_RM_AA_ZB_SUB_SURF2:			return "G_RM_AA_ZB_SUB_SURF2";
		case 	G_RM_AA_ZB_PCL_SURF:			return "G_RM_AA_ZB_PCL_SURF";
		case 	G_RM_AA_ZB_PCL_SURF2:			return "G_RM_AA_ZB_PCL_SURF2";
		case 	G_RM_AA_ZB_OPA_TERR:			return "G_RM_AA_ZB_OPA_TERR";
		case 	G_RM_AA_ZB_OPA_TERR2:			return "G_RM_AA_ZB_OPA_TERR2";
		case 	G_RM_AA_ZB_TEX_TERR:			return "G_RM_AA_ZB_TEX_TERR";
		case 	G_RM_AA_ZB_TEX_TERR2:			return "G_RM_AA_ZB_TEX_TERR2";
		case 	G_RM_AA_ZB_SUB_TERR:			return "G_RM_AA_ZB_SUB_TERR";
		case 	G_RM_AA_ZB_SUB_TERR2:			return "G_RM_AA_ZB_SUB_TERR2";
		case 	G_RM_RA_ZB_OPA_SURF:			return "G_RM_RA_ZB_OPA_SURF";
		case 	G_RM_RA_ZB_OPA_SURF2:			return "G_RM_RA_ZB_OPA_SURF2";
		case 	G_RM_RA_ZB_OPA_DECAL:			return "G_RM_RA_ZB_OPA_DECAL";
		case 	G_RM_RA_ZB_OPA_DECAL2:			return "G_RM_RA_ZB_OPA_DECAL2";
		case 	G_RM_RA_ZB_OPA_INTER:			return "G_RM_RA_ZB_OPA_INTER";
		case 	G_RM_RA_ZB_OPA_INTER2:			return "G_RM_RA_ZB_OPA_INTER2";
		case 	G_RM_AA_OPA_SURF:				return "G_RM_AA_OPA_SURF";
		case 	G_RM_AA_OPA_SURF2:				return "G_RM_AA_OPA_SURF2";
		case 	G_RM_AA_XLU_SURF:				return "G_RM_AA_XLU_SURF";
		case 	G_RM_AA_XLU_SURF2:				return "G_RM_AA_XLU_SURF2";
		case 	G_RM_AA_XLU_LINE:				return "G_RM_AA_XLU_LINE";
		case 	G_RM_AA_XLU_LINE2:				return "G_RM_AA_XLU_LINE2";
		case 	G_RM_AA_DEC_LINE:				return "G_RM_AA_DEC_LINE";
		case 	G_RM_AA_DEC_LINE2:				return "G_RM_AA_DEC_LINE2";
		case 	G_RM_AA_TEX_EDGE:				return "G_RM_AA_TEX_EDGE";
		case 	G_RM_AA_TEX_EDGE2:				return "G_RM_AA_TEX_EDGE2";
		case 	G_RM_AA_SUB_SURF:				return "G_RM_AA_SUB_SURF";
		case 	G_RM_AA_SUB_SURF2:				return "G_RM_AA_SUB_SURF2";
		case 	G_RM_AA_PCL_SURF:				return "G_RM_AA_PCL_SURF";
		case 	G_RM_AA_PCL_SURF2:				return "G_RM_AA_PCL_SURF2";
		case 	G_RM_AA_OPA_TERR:				return "G_RM_AA_OPA_TERR";
		case 	G_RM_AA_OPA_TERR2:				return "G_RM_AA_OPA_TERR2";
		case 	G_RM_AA_TEX_TERR:				return "G_RM_AA_TEX_TERR";
		case 	G_RM_AA_TEX_TERR2:				return "G_RM_AA_TEX_TERR2";
		case 	G_RM_AA_SUB_TERR:				return "G_RM_AA_SUB_TERR";
		case 	G_RM_AA_SUB_TERR2:				return "G_RM_AA_SUB_TERR2";
		case 	G_RM_RA_OPA_SURF:				return "G_RM_RA_OPA_SURF";
		case 	G_RM_RA_OPA_SURF2:				return "G_RM_RA_OPA_SURF2";
		case 	G_RM_ZB_OPA_SURF:				return "G_RM_ZB_OPA_SURF";
		case 	G_RM_ZB_OPA_SURF2:				return "G_RM_ZB_OPA_SURF2";
		case 	G_RM_ZB_XLU_SURF:				return "G_RM_ZB_XLU_SURF";
		case 	G_RM_ZB_XLU_SURF2:				return "G_RM_ZB_XLU_SURF2";
		case 	G_RM_ZB_OPA_DECAL:				return "G_RM_ZB_OPA_DECAL";
		case 	G_RM_ZB_OPA_DECAL2:				return "G_RM_ZB_OPA_DECAL2";
		case 	G_RM_ZB_XLU_DECAL:				return "G_RM_ZB_XLU_DECAL";
		case 	G_RM_ZB_XLU_DECAL2:				return "G_RM_ZB_XLU_DECAL2";
		case 	G_RM_ZB_CLD_SURF:				return "G_RM_ZB_CLD_SURF";
		case 	G_RM_ZB_CLD_SURF2:				return "G_RM_ZB_CLD_SURF2";
		case 	G_RM_ZB_OVL_SURF:				return "G_RM_ZB_OVL_SURF";
		case 	G_RM_ZB_OVL_SURF2:				return "G_RM_ZB_OVL_SURF2";
		case 	G_RM_ZB_PCL_SURF:				return "G_RM_ZB_PCL_SURF";
		case 	G_RM_ZB_PCL_SURF2:				return "G_RM_ZB_PCL_SURF2";
		case 	G_RM_OPA_SURF:					return "G_RM_OPA_SURF";
		case 	G_RM_OPA_SURF2:					return "G_RM_OPA_SURF2";
		case 	G_RM_XLU_SURF:					return "G_RM_XLU_SURF";
		case 	G_RM_XLU_SURF2:					return "G_RM_XLU_SURF2";
		case 	G_RM_CLD_SURF:					return "G_RM_CLD_SURF";
		case 	G_RM_CLD_SURF2:					return "G_RM_CLD_SURF2";
		case 	G_RM_TEX_EDGE:					return "G_RM_TEX_EDGE";
		case 	G_RM_TEX_EDGE2:					return "G_RM_TEX_EDGE2";
		case 	G_RM_PCL_SURF:					return "G_RM_PCL_SURF";
		case 	G_RM_PCL_SURF2:					return "G_RM_PCL_SURF2";
		case G_RM_ADD:							return "G_RM_ADD";
		case G_RM_ADD2:							return "G_RM_ADD2";
		case G_RM_NOOP:							return "G_RM_NOOP";
		//case G_RM_NOOP2:						return "G_RM_NOOP2";
		case G_RM_VISCVG:						return "G_RM_VISCVG";
		case G_RM_VISCVG2:						return "G_RM_VISCVG2";
		case G_RM_OPA_CI:						return "G_RM_OPA_CI";
		case G_RM_OPA_CI2:						return "G_RM_OPA_CI2";
		case G_RM_FOG_SHADE_A:					return "G_RM_FOG_SHADE_A";
		case G_RM_FOG_PRIM_A:					return "G_RM_FOG_PRIM_A";
	//	case G_RM_PASS:							return "G_RM_PASS";

		default:
			{
				switch ( mode & 0xff000000 )
				{
				case G_RM_FOG_SHADE_A:			return "G_RM_FOG_SHADE_A";
				case G_RM_FOG_PRIM_A:			return "G_RM_FOG_PRIM_A";
				case G_RM_PASS:					return "G_RM_PASS";
				}
				break;
			}
	}

	static char buffer[32];
	sprintf( buffer, "Unknown: %08x", mode  );
	return buffer;
}
#endif	// DAEDALUS_DEBUG_DISPLAYLIST

//*****************************************************************************
//
//*****************************************************************************
void	RDP_SetOtherMode( u32 cmd_hi, u32 cmd_lo )
{
	gRDPOtherMode._u64 = u64( cmd_hi ) << 32 | u64( cmd_lo );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		// High
		static const char *alphadithertypes[4]	= {"Pattern", "NotPattern", "Noise", "Disable"};
		static const char *rgbdithertype[4]		= {"MagicSQ", "Bayer", "Noise", "Disable"};
		static const char *convtype[8]			= {"Conv", "?", "?", "?",   "?", "FiltConv", "Filt", "?"};
		static const char *filtertype[4]		= {"Point", "?", "Bilinear", "Average"};
		static const char *textluttype[4]		= {"None", "?", "RGBA16", "IA16"};
		static const char *cycletype[4]			= {"1Cycle", "2Cycle", "Copy", "Fill"};
		static const char *detailtype[4]		= {"Clamp", "Sharpen", "Detail", "?"};
		static const char *alphacomptype[4]		= {"None", "Threshold", "?", "Dither"};
		static const char * szCvgDstMode[4]		= { "Clamp", "Wrap", "Full", "Save" };
		static const char * szZMode[4]			= { "Opa", "Inter", "XLU", "Decal" };
		static const char * szZSrcSel[2]		= { "Pixel", "Primitive" };

		u32 dwM1A_1 = (gRDPOtherMode.blender>>14) & 0x3;
		u32 dwM1B_1 = (gRDPOtherMode.blender>>10) & 0x3;
		u32 dwM2A_1 = (gRDPOtherMode.blender>>6) & 0x3;
		u32 dwM2B_1 = (gRDPOtherMode.blender>>2) & 0x3;

		u32 dwM1A_2 = (gRDPOtherMode.blender>>12) & 0x3;
		u32 dwM1B_2 = (gRDPOtherMode.blender>>8) & 0x3;
		u32 dwM2A_2 = (gRDPOtherMode.blender>>4) & 0x3;
		u32 dwM2B_2 = (gRDPOtherMode.blender   ) & 0x3;

		DL_PF( "    alpha_compare: %s", alphacomptype[ gRDPOtherMode.alpha_compare ]);
		DL_PF( "    depth_source:  %s", szZSrcSel[ gRDPOtherMode.depth_source ]);
		DL_PF( "    aa_en:         %d", gRDPOtherMode.aa_en );
		DL_PF( "    z_cmp:         %d", gRDPOtherMode.z_cmp );
		DL_PF( "    z_upd:         %d", gRDPOtherMode.z_upd );
		DL_PF( "    im_rd:         %d", gRDPOtherMode.im_rd );
		DL_PF( "    clr_on_cvg:    %d", gRDPOtherMode.clr_on_cvg );
		DL_PF( "    cvg_dst:       %s", szCvgDstMode[ gRDPOtherMode.cvg_dst ] );
		DL_PF( "    zmode:         %s", szZMode[ gRDPOtherMode.zmode ] );
		DL_PF( "    cvg_x_alpha:   %d", gRDPOtherMode.cvg_x_alpha );
		DL_PF( "    alpha_cvg_sel: %d", gRDPOtherMode.alpha_cvg_sel );
		DL_PF( "    force_bl:      %d", gRDPOtherMode.force_bl );
		DL_PF( "    tex_edge:      %d", gRDPOtherMode.tex_edge );
		DL_PF( "    blender:       %04x - %s*%s + %s*%s | %s*%s + %s*%s", gRDPOtherMode.blender,
										sc_szBlClr[dwM1A_1], sc_szBlA1[dwM1B_1], sc_szBlClr[dwM2A_1], sc_szBlA2[dwM2B_1],
										sc_szBlClr[dwM1A_2], sc_szBlA1[dwM1B_2], sc_szBlClr[dwM2A_2], sc_szBlA2[dwM2B_2]);
		DL_PF( "    blend_mask:    %d", gRDPOtherMode.blend_mask );
		DL_PF( "    alpha_dither:  %s", alphadithertypes[ gRDPOtherMode.alpha_dither ] );
		DL_PF( "    rgb_dither:    %s", rgbdithertype[ gRDPOtherMode.rgb_dither ] );
		DL_PF( "    comb_key:      %s", gRDPOtherMode.comb_key ? "Key" : "None" );
		DL_PF( "    text_conv:     %s", convtype[ gRDPOtherMode.text_conv ] );
		DL_PF( "    text_filt:     %s", filtertype[ gRDPOtherMode.text_filt ] );
		DL_PF( "    text_tlut:     %s", textluttype[ gRDPOtherMode.text_tlut ] );
		DL_PF( "    text_lod:      %s", gRDPOtherMode.text_lod ? "LOD": "Tile" );
		DL_PF( "    text_detail:   %s", detailtype[ gRDPOtherMode.text_detail ] );
		DL_PF( "    text_persp:    %s", gRDPOtherMode.text_persp ? "On" : "Off" );
		DL_PF( "    cycle_type:    %s", cycletype[ gRDPOtherMode.cycle_type ] );
		DL_PF( "    color_dither:  %d", gRDPOtherMode.color_dither );
		DL_PF( "    pipeline:      %s", gRDPOtherMode.pipeline ? "1Primitive" : "NPrimitive" );

		u32 c1_mode = (gRDPOtherMode.blender & 0xff00) << 16;
		u32 c2_mode = (gRDPOtherMode.blender & 0x00ff) << 16;

		DL_PF( "      %s", GetBlenderModeDescription( c1_mode ) );
		DL_PF( "      %s", GetBlenderModeDescription( c2_mode ) );
	}
#endif
}
