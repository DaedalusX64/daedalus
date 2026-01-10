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

#include "DLParser.h"
#include "PVRRenderer.h"
#include "Utility\Profiler.h"

#include "RDP.h"

#include "DebugDisplayList.h"

#include "Debug\Dump.h"

#include "Core\Memory.h"
#include "Core\ROM.h"
#include "Core\CPU.h"

#include "TextureCache.h"
#include "ConvertImage.h"			// Convert555ToRGBA

#include "ultra_sptask.h"
#include "ultra_gbi.h"
#include "ultra_rcp.h"

#include "daedalus_crc.h"
#include "DaedTiming.h"
#include "DaedIO.h"

#include "Debug\DBGConsole.h"

#include <vector>
#include "math/vmath.h"

// This is a hack for now - to be replace with correct instantiation.
#define mpRenderer PVRRenderer::Get()

static const u32	MAX_RAM_ADDRESS = (4*1024*1024);

const char *	gDisplayListRootPath = "DisplayLists";
const char *	gDisplayListDumpPathFormat = "dl%04d.txt";

bool	gSkipNextDisplayList = false;

u32 gTesselatorVerticesIn( 0 );
u32 gTesselatorVerticesOut( 0 );

#define N64COL_GETR( col )		(((col) >> 24)&0xFF)
#define N64COL_GETG( col )		(((col) >> 16)&0xFF)
#define N64COL_GETB( col )		(((col) >>  8)&0xFF)
#define N64COL_GETA( col )		(((col)      )&0xFF)

#define N64COL_GETR_F( col )	(N64COL_GETR(col) / 255.0f)
#define N64COL_GETG_F( col )	(N64COL_GETG(col) / 255.0f)
#define N64COL_GETB_F( col )	(N64COL_GETB(col) / 255.0f)
#define N64COL_GETA_F( col )	(N64COL_GETA(col) / 255.0f)

//*************************************************************************************
// Macros
//*************************************************************************************
#ifdef DAEDALUS_GFX_PLUGIN

inline void FinishRDPJob()
{
	*g_GraphicsInfo.xMI_INTR_REG |= MI_INTR_DP;
	g_GraphicsInfo.CheckInterrupts();
}

#else

inline void 	FinishRDPJob()
{
	Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_DP);
	CPU_AddJob(CPU_CHECK_INTERRUPTS);
}

#endif



#define MessageBox( w, m, t, n )		DAEDALUS_ASSERTMSG( m )

/*
#undef DL_PF
#define DL_PF DBGConsole_Msg2

inline void DBGConsole_Msg2( const char * szFormat, ... )
{
    char szBuffer[1000];

	va_list va;
	va_start(va, szFormat);
	vsprintf(szBuffer, szFormat, va);
	va_end(va);
	
	if ( CDebugConsole::IsAvailable() )
	{
		CDebugConsole::Get()->Msg( 0, "%s", szBuffer );
	}
	else
	{
	//	OutputDebugString( szBuffer );
	//	OutputDebugString( "\r\n" );	

	}
}
*/

//*****************************************************************************
// GBI2
//*****************************************************************************
#define	G_GBI2_RDPHALF_2		0xf1
#define	G_GBI2_SETOTHERMODE_H	0xe3
#define	G_GBI2_SETOTHERMODE_L	0xe2
#define	G_GBI2_RDPHALF_1		0xe1
#define	G_GBI2_SPNOOP			0xe0
#define	G_GBI2_ENDDL			0xdf
#define	G_GBI2_DL				0xde
#define	G_GBI2_LOAD_UCODE		0xdd
#define	G_GBI2_MOVEMEM			0xdc
#define	G_GBI2_MOVEWORD			0xdb
#define	G_GBI2_MTX				0xda
#define G_GBI2_GEOMETRYMODE		0xd9
#define	G_GBI2_POPMTX			0xd8
#define	G_GBI2_TEXTURE			0xd7
#define	G_GBI2_SUBMODULE		0xd6

#define	G_GBI2_NOOP				0x00
#define	G_GBI2_VTX				0x01
#define	G_GBI2_MODIFYVTX		0x02
#define	G_GBI2_CULLDL			0x03
#define	G_GBI2_BRANCH_Z			0x04
#define	G_GBI2_TRI1				0x05
#define G_GBI2_TRI2				0x06
#define G_GBI2_LINE3D			0x07


//*****************************************************************************
// GBI1
//*****************************************************************************
#define	G_GBI1_SPNOOP			0
#define	G_GBI1_MTX				1
#define G_GBI1_RESERVED0		2
#define G_GBI1_MOVEMEM			3
#define	G_GBI1_VTX				4
#define G_GBI1_RESERVED1		5
#define	G_GBI1_DL				6
#define G_GBI1_RESERVED2		7
#define G_GBI1_RESERVED3		8
#define G_GBI1_SPRITE2D_BASE	9

#define	G_GBI1_NOOP				0xc0	/*   0 */
#define	G_GBI1_TRI1				0xbf
#define G_GBI1_CULLDL			0xbe
#define	G_GBI1_POPMTX			0xbd
#define	G_GBI1_MOVEWORD			0xbc
#define	G_GBI1_TEXTURE			0xbb
#define	G_GBI1_SETOTHERMODE_H	0xba
#define	G_GBI1_SETOTHERMODE_L	0xb9
#define G_GBI1_ENDDL			0xb8
#define G_GBI1_SETGEOMETRYMODE	0xb7
#define G_GBI1_CLEARGEOMETRYMODE	0xb6
#define G_GBI1_LINE3D			0xb5
#define G_GBI1_RDPHALF_1		0xb4
#define G_GBI1_RDPHALF_2		0xb3
//#if (defined(F3DEX_GBI)||defined(F3DLP_GBI))
#  define G_GBI1_MODIFYVTX		0xb2
#  define G_GBI1_TRI2			0xb1
#  define G_GBI1_BRANCH_Z		0xb0
#  define G_GBI1_LOAD_UCODE		0xaf
//#else
#  define G_GBI1_RDPHALF_CONT	0xb2
//#endif

// Overloaded for sprite microcode
#define G_GBI1_SPRITE2D_SCALEFLIP    0xbe
#define G_GBI1_SPRITE2D_DRAW         0xbd




//*****************************************************************************
// RDP - Common to GBI1 and GBI2
//*****************************************************************************
#define	G_RDP_SETCIMG			0xff	/*  -1 */
#define	G_RDP_SETZIMG			0xfe	/*  -2 */
#define	G_RDP_SETTIMG			0xfd	/*  -3 */
#define	G_RDP_SETCOMBINE		0xfc	/*  -4 */
#define	G_RDP_SETENVCOLOR		0xfb	/*  -5 */
#define	G_RDP_SETPRIMCOLOR		0xfa	/*  -6 */
#define	G_RDP_SETBLENDCOLOR		0xf9	/*  -7 */
#define	G_RDP_SETFOGCOLOR		0xf8	/*  -8 */
#define	G_RDP_SETFILLCOLOR		0xf7	/*  -9 */
#define	G_RDP_FILLRECT			0xf6	/* -10 */
#define	G_RDP_SETTILE			0xf5	/* -11 */
#define	G_RDP_LOADTILE			0xf4	/* -12 */
#define	G_RDP_LOADBLOCK			0xf3	/* -13 */
#define	G_RDP_SETTILESIZE		0xf2	/* -14 */
#define	G_RDP_LOADTLUT			0xf0	/* -16 */
#define	G_RDP_RDPSETOTHERMODE	0xef	/* -17 */
#define	G_RDP_SETPRIMDEPTH		0xee	/* -18 */
#define	G_RDP_SETSCISSOR		0xed	/* -19 */
#define	G_RDP_SETCONVERT		0xec	/* -20 */
#define	G_RDP_SETKEYR			0xeb	/* -21 */
#define	G_RDP_SETKEYGB			0xea	/* -22 */
#define	G_RDP_RDPFULLSYNC		0xe9	/* -23 */
#define	G_RDP_RDPTILESYNC		0xe8	/* -24 */
#define	G_RDP_RDPPIPESYNC		0xe7	/* -25 */
#define	G_RDP_RDPLOADSYNC		0xe6	/* -26 */
#define G_RDP_TEXRECTFLIP		0xe5	/* -27 */
#define G_RDP_TEXRECT			0xe4	/* -28 */


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

typedef struct
{
	union
	{
		u64		_u64;
		u32		_u32[2];

		struct
		{
			u32		cmd1;
			u32		cmd0;
		};

		struct
		{
			int		: 32;
			int		: 24;
			unsigned int		cmd : 8;
		};
	};
} MicroCodeCommand;

typedef void ( * MicroCodeInstruction )( MicroCodeCommand command );

static void DLParser_SetuCode( u32 ucode);

static void RDP_EnableTexturing(u32 tile_idx);
static void DLParser_DumpVtxInfo(u32 address, u32 dwV0, u32 dwN);
static void DLParser_DumpVtxInfoDKR(u32 address, u32 dwV0, u32 dwN);

static u32 g_dwNumDListsCulled;
static u32 g_dwNumVertices;

static BOOL g_bRDPHalted = TRUE;

u32 gRDPTime = 0;
u32 gRDPFrame = 0;

//*****************************************************************************
//
//*****************************************************************************
u32 g_dwOtherModeL   = 0;
u32 g_dwOtherModeH   = 0;

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                    uCode Config                      //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
static u32 gUCode = 0;

// This is the multiplier applied to vertex indices. 
// For GBI0, it is 10.
// For GBI1/2 it is 2.
static u32 gVertexStride = 10;

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                      Dumping                         //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
static BOOL gDumpNextDisplayList = false;

FILE * gDisplayListFile = NULL;

#endif

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                     GFX State                        //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

struct N64Light
{
	u32 Colour;
	u32	ColourCopy;
	f32 x,y,z;			// Direction
	u32 pad;
};


static u32	g_dwSegment[16];
static N64Light  g_N64Lights[8];
SetImgInfo g_TI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };
static SetImgInfo g_CI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };
static SetImgInfo g_DI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };


class CRDPStateManager
{
public:

	CRDPStateManager();
	~CRDPStateManager();

	void		EnableTexturing( PVRRenderer * p_renderer, u32 psp_idx, u32 tile_idx ) const;

	const RDP_Tile & GetTile( u32 idx ) const 
	{
		return mTiles[ idx ];
	}

	const RDP_TileSize & GetTileSize( u32 idx ) const
	{
		return mTileSizes[ idx ];
	}

	void		SetTile( u32 idx, const RDP_Tile & tile )
	{
		if( mTiles[ idx ] != tile )
		{
			mTiles[ idx ] = tile;
			mTileTextureInfoValid[ idx ] = false;
		}
	}

	void		SetTileSize( u32 idx, const RDP_TileSize & tile_size )
	{
		if( mTileSizes[ idx ] != tile_size )
		{
			mTileSizes[ idx ] = tile_size;
			mTileTextureInfoValid[ idx ] = false;
		}
	}

	void		LoadBlock( u32 idx, u32 address, u32 pitch, bool swapped )
	{
		u32	tmem_address( mTiles[ idx ].tmem & 0xfff );
		mLoadAddresses[ tmem_address ] = address;
		mLoadPitches[ tmem_address ] = pitch;
		mLoadSwapped[ tmem_address ] = swapped;
		InvalidateAllTileTextureInfo();		// Can potentially invalidate all texture infos
	}

	void		LoadTile( u32 idx, u32 address, u32 pitch )
	{
		u32	tmem_address( mTiles[ idx ].tmem & 0xfff );
		mLoadAddresses[ tmem_address ] = address;
		mLoadPitches[ tmem_address ] = pitch;
		InvalidateAllTileTextureInfo();		// Can potentially invalidate all texture infos
	}

private:
	const TextureInfo &		GetTextureInfo( u32 idx ) const;
	CTexture *				GetTexture( u32 idx, const TextureInfo & ti ) const;
	void					InvalidateAllTileTextureInfo()
	{
		for( u32 i = 0; i < 8; ++i )
		{
			mTileTextureInfoValid[ i ] = false;
		}
	}

private:
	RDP_Tile				mTiles[ 8 ];
	RDP_TileSize			mTileSizes[ 8 ];

	u32						mLoadAddresses[ 4096 ];
	u32						mLoadPitches[ 4096 ];
	bool					mLoadSwapped[ 4096 ];

	mutable TextureInfo		mTileTextureInfo[ 8 ];
	mutable bool			mTileTextureInfoValid[ 8 ];		// Set to false if this needs rebuilding

	mutable TextureInfo		mLastTextureInfo;
	mutable CTexture *		mpLastTexture;
	mutable bool			mLastTextureValid;

	mutable u32				mNumReused;
	mutable u32				mNumLoaded;
};

CRDPStateManager::CRDPStateManager()
:	mpLastTexture( NULL )
,	mLastTextureValid( false )
,	mNumReused( 0 )
,	mNumLoaded( 0 )
{
	InvalidateAllTileTextureInfo();
}

CRDPStateManager::~CRDPStateManager()
{
	SAFE_RELEASE( mpLastTexture );
}

const TextureInfo & CRDPStateManager::GetTextureInfo( u32 idx ) const
{
	if( !mTileTextureInfoValid[ idx ] )
	{
		TextureInfo &			ti( mTileTextureInfo[ idx ] );

		const RDP_Tile &		rdp_tile( mTiles[ idx ] );
		const RDP_TileSize &	rdp_tilesize( mTileSizes[ idx ] );
		u32						tmem_address( rdp_tile.tmem & 0xfff );
		u32						address( mLoadAddresses[ tmem_address ] );
		u32						pitch( mLoadPitches[ tmem_address ] );

		bool	swapped = false;
		bool	translate_to_origin( false );

		if ( pitch == u32(~0) )
		{
			// HACK: In Mario, the texture for the Head bit is offset for some reason
			translate_to_origin = true;	
			swapped = mLoadSwapped[ tmem_address ];

			// It was a block load - the pitch is determined by the tile size
			switch(rdp_tile.size)
			{
				case G_IM_SIZ_4b:  pitch = (rdp_tile.line*8); break;
				case G_IM_SIZ_8b:  pitch = (rdp_tile.line*8); break;
				case G_IM_SIZ_16b: pitch = (rdp_tile.line*8); break;
				case G_IM_SIZ_32b: pitch = (rdp_tile.line*8)*2; break;
			}
		}
		else
		{
			// It was a tile - the pitch is set when the tile is loaded
		}

		u32		tile_top(rdp_tilesize.top / 4);
		u32		tile_left(rdp_tilesize.left / 4);
		u32		tile_right((rdp_tilesize.right / 4) + 1);
		u32		tile_bottom((rdp_tilesize.bottom / 4) + 1);


		ti.TmemAddress = rdp_tile.tmem;
		ti.TmemPalAddress = 0x100 + (rdp_tile.palette<<2);		// Want to advance 16x16bpp palette entries (i.e. 32 bytes into tmem for each palette), i.e. <<5. We shift <<3 on lookup, so only shift <<2 here

		ti.Address = address;
		ti.SetFormat( rdp_tile.format );
		ti.SetSize( rdp_tile.size );
		if(translate_to_origin)
		{
			ti.Left = 0;
			ti.Top = 0;	
		}
		else
		{
			ti.Left = tile_left;
			ti.Top = tile_top;
		}
		ti.Width = tile_right - tile_left;
		ti.Height = tile_bottom - tile_top;
		ti.Pitch = pitch;
		ti.SetTLutFormat( gRDPOtherMode.text_tlut << G_MDSFT_TEXTLUT );
		ti.SetSwapped( swapped );

		// Hack - Extreme-G specifies RGBA/8 textures, but they're really CI8
		if (ti.GetFormat() == G_IM_FMT_RGBA &&
			(ti.GetSize() == G_IM_SIZ_8b || ti.GetSize() == G_IM_SIZ_4b))
		{
			ti.SetFormat( G_IM_FMT_CI );
		}

		if (ti.GetFormat() == G_IM_FMT_CI)
		{
			if (ti.GetTLutFormat() == G_TT_NONE)
				ti.SetTLutFormat( G_TT_RGBA16 );		// Force RGBA

		}

		mTileTextureInfoValid[ idx ] = true;
	}

	return mTileTextureInfo[ idx ];
}

CTexture * CRDPStateManager::GetTexture( u32 idx, const TextureInfo & ti ) const
{
	if( mLastTextureValid )
	{
		if( ti.IsEqual( mLastTextureInfo ) )
		{
			return mpLastTexture;
		}
	}

	DL_PF("     *Enabling Texturing for Tile: %d", idx);
	DL_PF("     *Performing texture map load:");
	DL_PF("     *  (%d,%d) %dx%d",	ti.Left, ti.Top, ti.Width, ti.Height);
	DL_PF("     *  Pitch: %d, Addr: 0x%08x", ti.Pitch, ti.Address);

	// Check for 0 width/height textures
	if (ti.Width == 0 || ti.Height == 0)
	{
		DAEDALUS_ASSERTMSG( "Loading texture with 0 width/height" );
		return NULL;
	}

	mLastTextureInfo = ti;
	mpLastTexture = CTextureCache::Get()->GetTexture(&ti);
	mLastTextureValid = true;
	return mpLastTexture;
}

void CRDPStateManager::EnableTexturing( PVRRenderer * p_renderer, u32 psp_idx, u32 tile_idx ) const
{
	const TextureInfo &		ti( GetTextureInfo( tile_idx ) );

	CTexture *				pTexture( GetTexture( tile_idx, ti ) );
	if (pTexture != NULL)
	{	
		p_renderer->SetTexture( psp_idx, pTexture, ti.Left, ti.Top, ti.Width, ti.Height);

		PVRRenderer::ETextureAddressMode	mode_u;
		PVRRenderer::ETextureAddressMode	mode_v;

		//
		// Initialise the clamping state. When the mask is 0, it forces clamp mode.
		//
		const RDP_Tile &		rdp_tile( mTiles[ tile_idx ] );

		if (rdp_tile.clamp_s || rdp_tile.mask_s == 0)		mode_u = PVRRenderer::TAM_CLAMP;
		else if (rdp_tile.mirror_s)							mode_u = PVRRenderer::TAM_MIRROR;
		else												mode_u = PVRRenderer::TAM_WRAP;
		
		if (rdp_tile.clamp_t || rdp_tile.mask_t == 0)		mode_v = PVRRenderer::TAM_CLAMP;
		else if (rdp_tile.mirror_t)							mode_v = PVRRenderer::TAM_MIRROR;
		else												mode_v = PVRRenderer::TAM_WRAP;

		p_renderer->SetAddressMode( psp_idx, mode_u, mode_v );
	}
	else
	{
		p_renderer->SetTexture( psp_idx, NULL, 0, 0, 64, 64 );
	}
}


CRDPStateManager		gRDPStateManager;

//u32		gPalAddresses[ 4096 ];


// The display list PC stack. Before this was an array of 10
// items, but this way we can nest as deeply as necessary. 

typedef struct 
{
	u32 addr;
	u32 limit;
	// Push/pop?
} DList;

static std::vector< DList > g_dwPCStack;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
static u32					gCurrentInstructionCount = 0;			// Used for debugging display lists
static u32					gTotalInstructionCount = 0;
static u32					gInstructionCountLimit = UNLIMITED_INSTRUCTION_COUNT;
#endif

static BOOL  g_bTextureEnable = FALSE;
static u32 g_dwGeometryMode = 0;

static u32 g_dwAmbientLight = 0;

u32 gTextureTile = 0;
u32 gTextureLevel = 0;

static u32 g_dwFillColor		= 0xFFFFFFFF;

static u32 g_dwLastPurgeTimeTime = 0;		// Time textures were last purged


//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                      Strings                         //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

namespace
{
const char *gFormatNames[8] = {"RGBA", "YUV", "CI", "IA", "I", "?1", "?2", "?3"};
const char *gSizeNames[4]   = {"4bpp", "8bpp", "16bpp", "32bpp"};
const char *gOnOffNames[2]  = {"Off", "On"};

const char *gTLUTTypeName[4] = {"None", "?", "RGBA16", "IA16"};

const char * sc_colcombtypes32[32] =
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
const char *sc_colcombtypes16[16] =
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
const char *sc_colcombtypes8[8] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ", 
	"Shade       ", "Env         ",
	"1           ", "0           ",
};
}


// Mask down to 0x003FFFFF?
#define RDPSegAddr(seg) ( (g_dwSegment[((seg)>>24)&0x0F]&0x00ffffff) + ((seg)&0x00FFFFFF) )

//*****************************************************************************
// GBI1
//*****************************************************************************
static void DLParser_GBI0_Vtx( MicroCodeCommand command );
static void DLParser_GBI0_Tri2( MicroCodeCommand command );

static void DLParser_Nothing( MicroCodeCommand command );
static void DLParser_GBI1_SpNoop( MicroCodeCommand command );
static void DLParser_GBI1_Mtx( MicroCodeCommand command );
static void DLParser_MtxDKR( MicroCodeCommand command );
static void DLParser_GBI2_ModifyVtx( MicroCodeCommand command );
static void DLParser_GBI1_MoveMem( MicroCodeCommand command );
static void DLParser_GBI1_ModifyVtx( MicroCodeCommand command );

static void DLParser_GBI0_Vtx_DKR( MicroCodeCommand command );
static void DLParser_GBI1_Vtx( MicroCodeCommand command );
static void DLParser_GBI0_Vtx_WRUS( MicroCodeCommand command );

static void DLParser_DmaTri( MicroCodeCommand command );
static void DLParser_GBI1_DL( MicroCodeCommand command );
static void DLParser_DLInMem( MicroCodeCommand command );
static void DLParser_GBI1_Reserved0( MicroCodeCommand command );
static void DLParser_GBI1_Reserved1( MicroCodeCommand command );
static void DLParser_GBI1_Reserved2( MicroCodeCommand command );
static void DLParser_GBI1_Reserved3( MicroCodeCommand command );
static void DLParser_GBI1_Sprite2DBase( MicroCodeCommand command );
static void DLParser_GBI1_Sprite2DScaleFlip( MicroCodeCommand command );
static void DLParser_GBI1_Sprite2DDraw( MicroCodeCommand command );

template < int VertexStride > void DLParser_GBI1_Tri2_T( MicroCodeCommand command );
static void DLParser_GBI1_RDPHalf_Cont( MicroCodeCommand command );
static void DLParser_GBI1_RDPHalf_2( MicroCodeCommand command );
static void DLParser_GBI1_RDPHalf_1( MicroCodeCommand command );
template< int VertexStride > void DLParser_GBI1_Line3D_T( MicroCodeCommand command );
static void DLParser_GBI1_ClearGeometryMode( MicroCodeCommand command );
static void DLParser_GBI1_SetGeometryMode( MicroCodeCommand command );
static void DLParser_GBI1_EndDL( MicroCodeCommand command );
static void DLParser_GBI1_SetOtherModeL( MicroCodeCommand command );
static void DLParser_GBI1_SetOtherModeH( MicroCodeCommand command );
static void DLParser_GBI1_Texture( MicroCodeCommand command );
static void DLParser_GBI1_MoveWord( MicroCodeCommand command );
static void DLParser_GBI1_PopMtx( MicroCodeCommand command );
static void DLParser_GBI1_CullDL( MicroCodeCommand command );
template< int VertexStride > void DLParser_GBI1_Tri1_T( MicroCodeCommand command );
static void DLParser_GBI1_Noop( MicroCodeCommand command );
static void DLParser_GBI1_LoadUCode( MicroCodeCommand command );

//*****************************************************************************
// GBI2
//*****************************************************************************
static void DLParser_GBI2_Noop( MicroCodeCommand command );
static void DLParser_GBI2_SubModule( MicroCodeCommand command );
static void DLParser_GBI2_DL( MicroCodeCommand command );
static void DLParser_GBI2_CullDL( MicroCodeCommand command );
static void DLParser_GBI2_EndDL( MicroCodeCommand command );
static void DLParser_GBI2_MoveWord( MicroCodeCommand command );
static void DLParser_GBI2_Texture( MicroCodeCommand command );
static void DLParser_GBI2_GeometryMode( MicroCodeCommand command );
static void DLParser_GBI2_SetOtherModeL( MicroCodeCommand command );
static void DLParser_GBI2_SetOtherModeH( MicroCodeCommand command );
static void DLParser_GBI2_MoveMem( MicroCodeCommand command );
static void DLParser_GBI2_Mtx( MicroCodeCommand command );
static void DLParser_GBI2_PopMtx( MicroCodeCommand command );
static void DLParser_GBI2_Vtx( MicroCodeCommand command );
static void DLParser_GBI2_Tri2( MicroCodeCommand command );
static void DLParser_GBI2_Line3D( MicroCodeCommand command );
static void DLParser_GBI2_Tri1( MicroCodeCommand command );

static void DLParser_GBI2_RDPHalf_1( MicroCodeCommand command );
static void DLParser_GBI2_RDPHalf_2( MicroCodeCommand command );

static void DLParser_GBI1_BranchZ( MicroCodeCommand command );
static void DLParser_GBI2_SpNoop( MicroCodeCommand command );
static void DLParser_GBI2_LoadUCode( MicroCodeCommand command );
static void DLParser_GBI2_BranchZ( MicroCodeCommand command );

//*****************************************************************************
// RDP Commands
//*****************************************************************************
static void DLParser_TexRect( MicroCodeCommand command );
static void DLParser_TexRectFlip( MicroCodeCommand command );
static void DLParser_RDPLoadSync( MicroCodeCommand command );
static void DLParser_RDPPipeSync( MicroCodeCommand command );
static void DLParser_RDPTileSync( MicroCodeCommand command );
static void DLParser_RDPFullSync( MicroCodeCommand command );
static void DLParser_SetKeyGB( MicroCodeCommand command );
static void DLParser_SetKeyR( MicroCodeCommand command );
static void DLParser_SetConvert( MicroCodeCommand command );
static void DLParser_SetScissor( MicroCodeCommand command );
static void DLParser_SetPrimDepth( MicroCodeCommand command );
static void DLParser_RDPSetOtherMode( MicroCodeCommand command );
static void DLParser_LoadTLut( MicroCodeCommand command );
static void DLParser_SetTileSize( MicroCodeCommand command );
static void DLParser_LoadBlock( MicroCodeCommand command );
static void DLParser_LoadTile( MicroCodeCommand command );
static void DLParser_SetTile( MicroCodeCommand command );
static void DLParser_FillRect( MicroCodeCommand command );
static void DLParser_SetFillColor( MicroCodeCommand command );
static void DLParser_SetFogColor( MicroCodeCommand command );
static void DLParser_SetBlendColor( MicroCodeCommand command );
static void DLParser_SetPrimColor( MicroCodeCommand command );
static void DLParser_SetEnvColor( MicroCodeCommand command );
static void DLParser_SetCombine( MicroCodeCommand command );
static void DLParser_SetTImg( MicroCodeCommand command );
static void DLParser_SetZImg( MicroCodeCommand command );
static void DLParser_SetCImg( MicroCodeCommand command );


//*****************************************************************************
//
//*****************************************************************************
static LPCSTR gInstructionName[256];
static MicroCodeInstruction gInstructionLookup[256];

//DKR: 00229BA8: 05710080 001E4AF0 CMD G_DMATRI  Triangles 9 at 801E4AF0

//*****************************************************************************
//
//*****************************************************************************
bool DLParser_Initialise()
{
	g_dwLastPurgeTimeTime = gRDPTime;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	gDumpNextDisplayList = false;
	gDisplayListFile = NULL;
#endif

	//
	// Reset all the RDP registers
	//
	gRDPOtherMode._u64 = 0;
	gRDPOtherMode.pad = G_RDP_RDPSETOTHERMODE;
	gRDPOtherMode.blender = 0x0050;

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_Finalise()
{
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                      Logging                         //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*****************************************************************************
//
//*****************************************************************************
void DLParser_DumpNextDisplayList()
{
	gDumpNextDisplayList = true;
}
#endif

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*****************************************************************************
//
//*****************************************************************************
u32 DLParser_GetTotalInstructionCount()
{
	return gTotalInstructionCount;
}
#endif

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetInstructionCountLimit( u32 limit )
{
	gInstructionCountLimit = limit;
}
#endif

//*****************************************************************************
// Identical to the above, but uses DL_PF
//*****************************************************************************
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
static void DLParser_DumpMux( u64 mux )
{
	u32 mux0 = (u32)(mux>>32);
	u32 mux1 = (u32)(mux);
	
	u32 aRGB0  = (mux0>>20)&0x0F;	// c1 c1		// a0
	u32 bRGB0  = (mux1>>28)&0x0F;	// c1 c2		// b0
	u32 cRGB0  = (mux0>>15)&0x1F;	// c1 c3		// c0
	u32 dRGB0  = (mux1>>15)&0x07;	// c1 c4		// d0

	u32 aA0    = (mux0>>12)&0x07;	// c1 a1		// Aa0
	u32 bA0    = (mux1>>12)&0x07;	// c1 a2		// Ab0
	u32 cA0    = (mux0>>9 )&0x07;	// c1 a3		// Ac0
	u32 dA0    = (mux1>>9 )&0x07;	// c1 a4		// Ad0

	u32 aRGB1  = (mux0>>5 )&0x0F;	// c2 c1		// a1
	u32 bRGB1  = (mux1>>24)&0x0F;	// c2 c2		// b1
	u32 cRGB1  = (mux0    )&0x1F;	// c2 c3		// c1
	u32 dRGB1  = (mux1>>6 )&0x07;	// c2 c4		// d1
	
	u32 aA1    = (mux1>>21)&0x07;	// c2 a1		// Aa1
	u32 bA1    = (mux1>>3 )&0x07;	// c2 a2		// Ab1
	u32 cA1    = (mux1>>18)&0x07;	// c2 a3		// Ac1
	u32 dA1    = (mux1    )&0x07;	// c2 a4		// Ad1

	DL_PF("    Mux: 0x%08x%08x", mux0, mux1);

	DL_PF("    RGB0: (%s - %s) * %s + %s", sc_colcombtypes16[aRGB0], sc_colcombtypes16[bRGB0], sc_colcombtypes32[cRGB0], sc_colcombtypes8[dRGB0]);		
	DL_PF("    A0  : (%s - %s) * %s + %s", sc_colcombtypes8[aA0], sc_colcombtypes8[bA0], sc_colcombtypes8[cA0], sc_colcombtypes8[dA0]);
	DL_PF("    RGB1: (%s - %s) * %s + %s", sc_colcombtypes16[aRGB1], sc_colcombtypes16[bRGB1], sc_colcombtypes32[cRGB1], sc_colcombtypes8[dRGB1]);		
	DL_PF("    A1  : (%s - %s) * %s + %s", sc_colcombtypes8[aA1],  sc_colcombtypes8[bA1], sc_colcombtypes8[cA1],  sc_colcombtypes8[dA1]);
}
#endif


//*****************************************************************************
//
//*****************************************************************************
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
static void	DLParser_DumpTaskInfo( const OSTask * pTask )
{
	DL_PF( "Task:         %08x", pTask->t.type  );
	DL_PF( "Flags:        %08x", pTask->t.flags  );
	DL_PF( "BootCode:     %08x", pTask->t.ucode_boot  );
	DL_PF( "BootCodeSize: %08x", pTask->t.ucode_boot_size  );

	DL_PF( "uCode:        %08x", pTask->t.ucode );
	DL_PF( "uCodeSize:    %08x", pTask->t.ucode_size );
	DL_PF( "uCodeData:    %08x", pTask->t.ucode_data );
	DL_PF( "uCodeDataSize:%08x", pTask->t.ucode_data_size );

	DL_PF( "Stack:        %08x", pTask->t.dram_stack );
	DL_PF( "StackS:       %08x", pTask->t.dram_stack_size );
	DL_PF( "Output:       %08x", pTask->t.output_buff );
	DL_PF( "OutputS:      %08x", pTask->t.output_buff_size );

	DL_PF( "Data( PC ):   %08x", pTask->t.data_ptr );
	DL_PF( "DataSize:     %08x", pTask->t.data_size );
	DL_PF( "YieldData:    %08x", pTask->t.yield_data_ptr );
	DL_PF( "YieldDataSize:%08x", pTask->t.yield_data_size );
}
#endif

//*****************************************************************************
//
//
//
//
//*****************************************************************************

// F3DEX		Fast3D EX
// F3DLX		Improved performance version of F3DEX, GBI compatibility. Can turn clipping on/off
// F3DLP		? Line ?
// F3DZEX		? Zelda ?

// NoN			No Near clipping
// Rej			Reject polys with one or more points outside screenspace

//F3DEX: Extended fast3d. Vertex cache is 32, up to 18 DL links
//F3DLX: Compatible with F3DEX, GBI, but not sub-pixel accurate. Clipping can be explicitly enabled/disabled
//F3DLX.Rej: No clipping, rejection instead. Vertex cache is 64
//F3FLP.Rej: Like F3FLX.Rej. Vertex cache is 80
//L3DEX: Line processing, Vertex cache is 32.

#define G_DMATRI	0x05
#define G_DLINMEM	0x07

// THE LEGEND OF ZELDA CZLE
// GOLDENEYE       NGEE
// TETRISPHERE     .......NTPP.
CHAR gLastMicrocodeString[ 300 ] = "DUMMYSTRING";		// If we leave this as an empty string, for ROMs with no RSP string, we get a false match

typedef struct
{
	u32 ucode;
	u32 crc_size;
	u32 crc_800;
	const CHAR * ucode_name;
} MicrocodeData;

// All star Tennis 99
// ArmyMen - Air Combat - Conflicts with Xena??
// Duke Nukem 64
// International Superstar Soccer '98
// Mischief Makers
// Testrisphere
// I S S 64

//Wipeout:
//Ucode is: 0xc705c37c, RSP Gfx ucode F3DLX         1.21 Yoshitaka Yasumoto Nintendo.
//Ucode is: 0xb65aa2da, RSP Gfx ucode L3DEX         1.21 Yoshitaka Yasumoto Nintendo.
//Ucode is: 0x700de42e, RSP SW Version: 2.0H, 02-12-97

//Flying Dragon:
//Ucode is: 0x1b304a74, RSP SW Version: 2.0H, 02-12-97
//Ucode is: 0xa56cf996, RSP Gfx ucode L3DEX         1.23 Yoshitaka Yasumoto Nintendo.
//Ucode is: 0xfc6529aa, RSP Gfx ucode F3DEX         1.23 Yoshitaka Yasumoto Nintendo.
//Ucode is: 0xca8927a0, RSP Gfx ucode F3DLX.Rej     1.23 Yoshitaka Yasumoto Nintendo.

//*****************************************************************************
//
//*****************************************************************************
static MicrocodeData g_MicrocodeData[] = 
{
	// SGI U64 GFX SW TEAM: S Anderson, S Carr, H Cheng, K Luster, R Moore, N Pooley, A Srinivasan
	{0, 0x150c3ce8, 0x150c3ce8, "RSP SW Version: 2.0D, 04-01-96"}, // Super Mario 64
	// Note ucode 4 - no idea why this is so different!
	{4, 0x2b94276f, 0x2b94276f, "RSP SW Version: 2.0D, 04-01-96"}, // Wave Race 64 (v1.0)
	{0, 0xb1870454, 0xb1870454, "RSP SW Version: 2.0D, 04-01-96"}, // Star Wars - Shadows of the Empire (v1.0), 
	{0, 0x51671ae4, 0x51671ae4, "RSP SW Version: 2.0D, 04-01-96"}, // Pilot Wings 64, 
	{0, 0x67b5ac55, 0x67b5ac55, "RSP SW Version: 2.0D, 04-01-96"}, // Wibble, 
	{0, 0x64dc8104, 0x64dc8104, "RSP SW Version: 2.0D, 04-01-96"}, // Dark Rift, 
	{0, 0xc02ac7bc, 0xc02ac7bc, "RSP SW Version: 2.0G, 09-30-96"}, // GoldenEye 007, 
	{0, 0xe5fee3bc, 0xe5fee3bc, "RSP SW Version: 2.0G, 09-30-96"}, // Aero Fighters Assault, 
	{0, 0x72109ec6, 0x72109ec6, "RSP SW Version: 2.0H, 02-12-97"}, // Duke Nukem 64, 
	{0, 0xf24a9a04, 0xf24a9a04, "RSP SW Version: 2.0H, 02-12-97"}, // Tetrisphere, 
	{0, 0x700de42e, 0x700de42e, "RSP SW Version: 2.0H, 02-12-97"}, // Wipeout 64 (uses GBI1 too!), 
	{0, 0x1b304a74, 0x1b304a74, "RSP SW Version: 2.0H, 02-12-97"}, // Flying Dragon, 
	{0, 0xe466b5bd, 0xe466b5bd, ""}, // Dark Rift, 

	// GBI1
	{1, 0x6d2a01b1, 0x6d2a01b1, "RSP Gfx ucode ZSortp 0.33 Yoshitaka Yasumoto Nintendo."}, // Mia Hamm Soccer 64, 

	{1, 0x45ca328e, 0x45ca328e, "RSP Gfx ucode F3DLX         0.95 Yoshitaka Yasumoto Nintendo."}, // Mario Kart 64, 
	{1, 0x98e3b909, 0x98e3b909, "RSP Gfx ucode F3DEX         0.95 Yoshitaka Yasumoto Nintendo."},	// Mario Kart 64
	{1, 0x5d446090, 0x5d446090, "RSP Gfx ucode F3DLP.Rej     0.96 Yoshitaka Yasumoto Nintendo."}, // Jikkyou J. League Perfect Striker, 
	{1, 0x244f5ca3, 0x244f5ca3, "RSP Gfx ucode F3DEX         1.00 Yoshitaka Yasumoto Nintendo."}, // F-1 Pole Position 64, 
	{1, 0x6a022585, 0x6a022585, "RSP Gfx ucode F3DEX.NoN     1.00 Yoshitaka Yasumoto Nintendo."}, // Turok - The Dinosaur Hunter (v1.0), 
	{1, 0x150706be, 0x150706be, "RSP Gfx ucode F3DLX.NoN     1.00 Yoshitaka Yasumoto Nintendo."}, // Extreme-G, 
	{1, 0x503f2c53, 0x503f2c53, "RSP Gfx ucode F3DEX.NoN     1.21 Yoshitaka Yasumoto Nintendo."}, // Bomberman 64, 
	{1, 0xc705c37c, 0xc705c37c, "RSP Gfx ucode F3DLX         1.21 Yoshitaka Yasumoto Nintendo."}, // Fighting Force 64, Wipeout 64
	{1, 0xa2146075, 0xa2146075, "RSP Gfx ucode F3DLX.NoN     1.21 Yoshitaka Yasumoto Nintendo."}, // San Francisco Rush - Extreme Racing, 
	{1, 0xb65aa2da, 0xb65aa2da, "RSP Gfx ucode L3DEX         1.21 Yoshitaka Yasumoto Nintendo."}, // Wipeout 64, 
	// Same crc, different ids!
	{1, 0xaebeda7d, 0xaebeda7d, "RSP Gfx ucode F3DLX.Rej     1.21 Yoshitaka Yasumoto Nintendo."}, // Jikkyou World Soccer 3, 
	{1, 0xaebeda7d, 0xaebeda7d, "RSP Gfx ucode F3DLX.Rej     1.23 Yoshitaka Yasumoto Nintendo."}, // Flying Dragon Fighters (Hiro no Ken Twin), 
	{1, 0x0c8e5ec9, 0x0c8e5ec9, "RSP Gfx ucode F3DEX         1.23 Yoshitaka Yasumoto Nintendo."}, // Wave Race 64 Shindou Edition
	{1, 0xfc6529aa, 0xfc6529aa, "RSP Gfx ucode F3DEX         1.23 Yoshitaka Yasumoto Nintendo."}, // Superman - The Animated Series, 
	{1, 0xa56cf996, 0xa56cf996, "RSP Gfx ucode L3DEX         1.23 Yoshitaka Yasumoto Nintendo."}, // Flying Dragon, 
	{1, 0xcc83b43f, 0xcc83b43f, "RSP Gfx ucode F3DEX.NoN     1.23 Yoshitaka Yasumoto Nintendo."}, // AeroGauge, 
	{1, 0xca8927a0, 0xca8927a0, "RSP Gfx ucode F3DLX.Rej     1.23 Yoshitaka Yasumoto Nintendo."},	// Puzzle Bobble 64, 
	{1, 0xd2d747b7, 0xd2d747b7, "RSP Gfx ucode F3DLX.NoN     1.23 Yoshitaka Yasumoto Nintendo."}, // Penny Racers, 
	{1, 0x74583614, 0x74583614, ""}, // Star Wars - Rogue Squadron, 

	// Sprite! See also VRally '99
	{1, 0xecd8b772, 0xecd8b772, "RSP Gfx ucode S2DEX  1.06 Yoshitaka Yasumoto Nintendo."}, // Yoshi's Story, 
	{1, 0xf59132f5, 0xf59132f5, "RSP Gfx ucode S2DEX  1.07 Yoshitaka Yasumoto Nintendo."}, // Bakuretsu Muteki Bangaioh, 
	
	// GBI2
	{5, 0xa8050bd1, 0xa8050bd1, "RSP Gfx ucode F3DEX       fifo 2.03  Yoshitaka Yasumoto 1998 Nintendo."}, // F-Zero X, 
	{5, 0x4e8055f0, 0x4e8055f0, "RSP Gfx ucode F3DLX.Rej   fifo 2.03  Yoshitaka Yasumoto 1998 Nintendo."}, // F-Zero X, 
	{5, 0xabf001f5, 0xabf001f5, "RSP Gfx ucode F3DFLX.Rej  fifo 2.03F Yoshitaka Yasumoto 1998 Nintendo."}, // F-Zero X, 

	{5, 0xadb4b686, 0xadb4b686, "RSP Gfx ucode F3DEX       fifo 2.04  Yoshitaka Yasumoto 1998 Nintendo."}, // Top Gear Rally 2, 
	{5, 0x779e2a9b, 0x779e2a9b, "RSP Gfx ucode F3DEX.NoN   fifo 2.04  Yoshitaka Yasumoto 1998 Nintendo."}, // California Speed, 
	{5, 0xa8cb3e09, 0xa8cb3e09, "RSP Gfx ucode L3DEX       fifo 2.04  Yoshitaka Yasumoto 1998 Nintendo."}, // In-Fisherman Bass Hunter 64, 
	{5, 0x2a1341d6, 0x2a1341d6, "RSP Gfx ucode F3DEX       fifo 2.04H Yoshitaka Yasumoto 1998 Nintendo."}, // Kirby 64 - The Crystal Shards, 
	{5, 0x4964b75d, 0x4964b75d, "RSP Gfx ucode F3DEX.NoN   fifo 2.05  Yoshitaka Yasumoto 1998 Nintendo."}, // Carmageddon 64 (uncensored), 
	{5, 0x39e3e95a, 0x39e3e95a, "RSP Gfx ucode F3DEX       fifo 2.05  Yoshitaka Yasumoto 1998 Nintendo."}, // Knife Edge - Nose Gunner, 
	{5, 0xd2913522, 0xd2913522, "RSP Gfx ucode F3DAM       fifo 2.05  Yoshitaka Yasumoto 1998 Nintendo."}, // Hey You, Pikachu!, 
	{5, 0x595a88de, 0x595a88de, "RSP Gfx ucode F3DEX.Rej   fifo 2.06  Yoshitaka Yasumoto 1998 Nintendo."}, // Bio Hazard 2, 
	{5, 0x0259f764, 0x0259f764, "RSP Gfx ucode F3DLX.Rej   fifo 2.06  Yoshitaka Yasumoto 1998 Nintendo."}, // Mario Party, 
	{5, 0xe1a5477a, 0xe1a5477a, "RSP Gfx ucode F3DEX.NoN   xbus 2.06  Yoshitaka Yasumoto 1998 Nintendo."}, // Command & Conquer, 
	{5, 0x4cfa0a19, 0x4cfa0a19, "RSP Gfx ucode F3DZEX.NoN  fifo 2.06H Yoshitaka Yasumoto 1998 Nintendo."}, // The Legend of Zelda - Ocarina of Time (v1.0), 
	{5, 0xdeb1cac0, 0xdeb1cac0, "RSP Gfx ucode F3DEX.NoN   fifo 2.07  Yoshitaka Yasumoto 1998 Nintendo."}, // Knockout Kings 2000, 
	{5, 0xf4184a7d, 0xf4184a7d, "RSP Gfx ucode F3DEX       fifo 2.07  Yoshitaka Yasumoto 1998 Nintendo."}, // Xena Warrior Princess - Talisman of Fate, Army Men - Air Combat, Destruction Derby
	{5, 0x4b013e60, 0x4b013e60, "RSP Gfx ucode F3DEX       xbus 2.07  Yoshitaka Yasumoto 1998 Nintendo."}, // Lode Runner 3-D, 
	{5, 0xd1a63836, 0xd1a63836, "RSP Gfx ucode L3DEX       fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo."}, // Hey You, Pikachu!, 
	{5, 0x97193667, 0x97193667, "RSP Gfx ucode F3DEX       fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo."}, // Top Gear Hyper-Bike, 
	{5, 0x92149ba8, 0x92149ba8, "RSP Gfx ucode F3DEX       fifo 2.08  Yoshitaka Yasumoto/Kawasedo 1999."}, // Paper Mario, 
	{5, 0xae0fb88f, 0xae0fb88f, "RSP Gfx ucode F3DEX       xbus 2.08  Yoshitaka Yasumoto 1999 Nintendo."}, // WWF WrestleMania 2000, 
	{5, 0xc572f368, 0xc572f368, "RSP Gfx ucode F3DLX.Rej   xbus 2.08  Yoshitaka Yasumoto 1999 Nintendo."}, // WWF No Mercy,
	{5, 0x9c2edb70, 0xea98e740, "RSP Gfx ucode F3DEX.NoN   fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo."}, // LEGO Racers, 
	{5, 0xea98e740, 0xea98e740, "RSP Gfx ucode F3DEX.NoN   fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo."}, // Bass Hunter 64, 
	{5, 0x79e004a6, 0x79e004a6, "RSP Gfx ucode F3DLX.Rej   fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo."}, // Mario Party 2, 
	{5, 0xaa6ab3ca, 0xaa6ab3ca, "RSP Gfx ucode F3DEX.Rej   fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo."}, // V-Rally Edition 99, 
	{5, 0x2c597e0f, 0x2c597e0f, "RSP Gfx ucode F3DEX       fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo."}, // Cruis'n Exotica,
	{5, 0x4e5f3e3b, 0x4e5f3e3b, "RSP Gfx ucode F3DEXBG.NoN fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo."}, // Conker's Bad Fur Day, 
	{5, 0x61f31862, 0x61f31862, "RSP Gfx ucode F3DEX.NoN   fifo 2.08H Yoshitaka Yasumoto 1999 Nintendo."}, // Pokemon Snap, 
	{5, 0x005f5b71, 0x005f5b71, "RSP Gfx ucode F3DZEX.NoN  fifo 2.08I Yoshitaka Yasumoto/Kawasedo 1999."}, // The Legend of Zelda 2 - Majora's Mask, 
	{5, 0x7064a163, 0x7064a163, ""}, // Perfect Dark (v1.0), 
	{5, 0x7b685972, 0x57b8095a, ""}, // Stunt Racer 64, 

	// Sprite!
	// Same crc, different ids!
	{5, 0x41839d1e, 0x41839d1e, "RSP Gfx ucode S2DEX       fifo 2.05  Yoshitaka Yasumoto 1998 Nintendo."}, // Chou Snobow Kids, 
	{5, 0x41839d1e, 0x41839d1e, "RSP Gfx ucode S2DEX       fifo 2.06  Yoshitaka Yasumoto 1998 Nintendo."}, // Bio Hazard 2, 
	{5, 0x41839d1e, 0x41839d1e, "RSP Gfx ucode S2DEX       fifo 2.07  Yoshitaka Yasumoto 1998 Nintendo."}, // Jinsei Game 64, 
	{5, 0xec89e273, 0xec89e273, "RSP Gfx ucode S2DEX       fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo."}, // V-Rally Edition 99, 
	{5, 0x5a72397b, 0xec89e273, "RSP Gfx ucode S2DEX       fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo."}, // Ogre Battle 64 - Person of Lordly Caliber, 

											{6, 0x6aef74f8, 0x6aef74f8, ""}, // Diddy Kong Racing (v1.0), 
	{6, 0x4c4eead8, 0x4c4eead8, ""}, // Diddy Kong Racing (v1.1), 

	{1, 0xed421e9a, 0xed421e9a, ""}, // Kuiki Uhabi Suigo, 
	{5, 0x37751932, 0x55c0fd25, ""}, // Bio Hazard 2, 
	{1, 0xa486bed3, 0xa486bed3, ""}, // Last Legion UX, 
	{1, 0xbe0b83e7, 0xbe0b83e7, ""}, // Jet Force Gemini, 

};


//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetuCode( u32 ucode )
{
	//DBGConsole_Msg(0, "[GReconfiguring RDP to process ucode %d]", ucode);

	enum GBIVersion
	{
		GBI_1 = 0,
		GBI_2
	};

	enum UCodeVersion
	{
		FAST3D,
		F3DEX,
		F3FLX,
		F3DLP,
		
	};

	GBIVersion ver = ucode == 5 ? GBI_2 : GBI_1;
	UCodeVersion ucode_ver = ucode == 0 ? FAST3D : F3DEX;

	for ( u32 i = 0; i < 256; i++ )
	{
		gInstructionLookup[ i ] = DLParser_Nothing;
		gInstructionName[ i ] = "???";
	}
	
#define SetCommand( cmd, func )					\
		gInstructionLookup[ cmd ] = func;		\
		gInstructionName[ cmd ] = #cmd;
		
	MicroCodeInstruction	DLParser_GBI1_Tri1_Instruction;
	MicroCodeInstruction	DLParser_GBI1_Tri2_Instruction;
	MicroCodeInstruction	DLParser_GBI1_Line3D_Instruction;
	switch( ucode )
	{
	case 0:
		gVertexStride = 10;
		DLParser_GBI1_Tri1_Instruction = DLParser_GBI1_Tri1_T< 10 >;
		DLParser_GBI1_Tri2_Instruction = DLParser_GBI1_Tri2_T< 10 >;
		DLParser_GBI1_Line3D_Instruction = DLParser_GBI1_Line3D_T< 10 >;
		break;
	case 4:
		gVertexStride = 5;	
		DLParser_GBI1_Tri1_Instruction = DLParser_GBI1_Tri1_T< 5 >;
		DLParser_GBI1_Tri2_Instruction = DLParser_GBI1_Tri2_T< 5 >;
		DLParser_GBI1_Line3D_Instruction = DLParser_GBI1_Line3D_T< 5 >;
		break;
	case 6:	
		gVertexStride = 10;	
		DLParser_GBI1_Tri1_Instruction = DLParser_GBI1_Tri1_T< 10 >;
		DLParser_GBI1_Tri2_Instruction = DLParser_GBI1_Tri2_T< 10 >;
		DLParser_GBI1_Line3D_Instruction = DLParser_GBI1_Line3D_T< 10 >;
		break;
	default:
		gVertexStride = 2;	
		DLParser_GBI1_Tri1_Instruction = DLParser_GBI1_Tri1_T< 2 >;
		DLParser_GBI1_Tri2_Instruction = DLParser_GBI1_Tri2_T< 2 >;
		DLParser_GBI1_Line3D_Instruction = DLParser_GBI1_Line3D_T< 2 >;
		break;
	}

	SetCommand( 0x0b			, DLParser_GBI1_SpNoop );
	SetCommand( 0x0a			, DLParser_GBI1_SpNoop );
	SetCommand( 0x09			, DLParser_GBI1_SpNoop );
	SetCommand( 0x08			, DLParser_GBI1_SpNoop );


	SetCommand( G_RDP_SETCIMG			, DLParser_SetCImg );
	SetCommand( G_RDP_SETZIMG			, DLParser_SetZImg );
	SetCommand( G_RDP_SETTIMG			, DLParser_SetTImg );
	SetCommand( G_RDP_SETCOMBINE		, DLParser_SetCombine );
	SetCommand( G_RDP_SETENVCOLOR		, DLParser_SetEnvColor );
	SetCommand( G_RDP_SETPRIMCOLOR		, DLParser_SetPrimColor );
	SetCommand( G_RDP_SETBLENDCOLOR		, DLParser_SetBlendColor );
	SetCommand( G_RDP_SETFOGCOLOR		, DLParser_SetFogColor );
	SetCommand( G_RDP_SETFILLCOLOR		, DLParser_SetFillColor );
	SetCommand( G_RDP_FILLRECT			, DLParser_FillRect );
	SetCommand( G_RDP_SETTILE			, DLParser_SetTile );
	SetCommand( G_RDP_LOADTILE			, DLParser_LoadTile );
	SetCommand( G_RDP_LOADBLOCK			, DLParser_LoadBlock );
	SetCommand( G_RDP_SETTILESIZE		, DLParser_SetTileSize );
	SetCommand( G_RDP_LOADTLUT			, DLParser_LoadTLut );
	SetCommand( G_RDP_RDPSETOTHERMODE	, DLParser_RDPSetOtherMode );
	SetCommand( G_RDP_SETPRIMDEPTH		, DLParser_SetPrimDepth );
	SetCommand( G_RDP_SETSCISSOR		, DLParser_SetScissor );
	SetCommand( G_RDP_SETCONVERT		, DLParser_SetConvert );
	SetCommand( G_RDP_SETKEYR			, DLParser_SetKeyR );
	SetCommand( G_RDP_SETKEYGB			, DLParser_SetKeyGB );
	SetCommand( G_RDP_RDPFULLSYNC		, DLParser_RDPFullSync );
	SetCommand( G_RDP_RDPTILESYNC		, DLParser_RDPTileSync );
	SetCommand( G_RDP_RDPPIPESYNC		, DLParser_RDPPipeSync );
	SetCommand( G_RDP_RDPLOADSYNC		, DLParser_RDPLoadSync );
	SetCommand( G_RDP_TEXRECTFLIP		, DLParser_TexRectFlip );
	SetCommand( G_RDP_TEXRECT			, DLParser_TexRect );

	if ( ver == GBI_1 )
	{
		// DMA commands
		SetCommand( G_GBI1_SPNOOP        , DLParser_GBI1_SpNoop );
		SetCommand( G_GBI1_MTX	         , DLParser_GBI1_Mtx );
		SetCommand( G_GBI1_RESERVED0     , DLParser_GBI1_Reserved0 );
		SetCommand( G_GBI1_MOVEMEM	     , DLParser_GBI1_MoveMem );
		SetCommand( G_GBI1_VTX	         , DLParser_GBI1_Vtx );
		SetCommand( G_GBI1_RESERVED1	 , DLParser_GBI1_Reserved1 );
		SetCommand( G_GBI1_DL			 , DLParser_GBI1_DL );
		SetCommand( G_GBI1_RESERVED2	 , DLParser_GBI1_Reserved2 );
		SetCommand( G_GBI1_RESERVED3	 , DLParser_GBI1_Reserved3 );
		SetCommand( G_GBI1_SPRITE2D_BASE , DLParser_GBI1_Sprite2DBase );

		// IMMEDIATE commands
		SetCommand( G_GBI1_NOOP          , DLParser_GBI1_Noop );
		
		
		SetCommand( G_GBI1_TRI1					, DLParser_GBI1_Tri1_Instruction );
		SetCommand( G_GBI1_CULLDL				, DLParser_GBI1_CullDL );
		SetCommand( G_GBI1_POPMTX				, DLParser_GBI1_PopMtx );
		SetCommand( G_GBI1_MOVEWORD				, DLParser_GBI1_MoveWord );
		SetCommand( G_GBI1_TEXTURE				, DLParser_GBI1_Texture );
		SetCommand( G_GBI1_SETOTHERMODE_H		, DLParser_GBI1_SetOtherModeH );
		SetCommand( G_GBI1_SETOTHERMODE_L		, DLParser_GBI1_SetOtherModeL );
		SetCommand( G_GBI1_ENDDL				, DLParser_GBI1_EndDL );
		SetCommand( G_GBI1_SETGEOMETRYMODE		, DLParser_GBI1_SetGeometryMode );
		SetCommand( G_GBI1_CLEARGEOMETRYMODE	, DLParser_GBI1_ClearGeometryMode );
		SetCommand( G_GBI1_LINE3D				, DLParser_GBI1_Line3D_Instruction );
		SetCommand( G_GBI1_RDPHALF_1			, DLParser_GBI1_RDPHalf_1 );
		SetCommand( G_GBI1_RDPHALF_2			, DLParser_GBI1_RDPHalf_2 );

		if ( ucode_ver == F3DEX || ucode_ver == F3DLP )
		{
			SetCommand( G_GBI1_MODIFYVTX			, DLParser_GBI1_ModifyVtx );
			SetCommand( G_GBI1_TRI2					, DLParser_GBI1_Tri2_Instruction );
			SetCommand( G_GBI1_BRANCH_Z				, DLParser_GBI1_BranchZ );
			SetCommand( G_GBI1_LOAD_UCODE			, DLParser_GBI1_LoadUCode );
		}
		else
		{
			SetCommand( G_GBI1_RDPHALF_CONT			, DLParser_GBI1_RDPHalf_Cont );
		}

		// Overloaded for sprite microcode
		//SetCommand( G_GBI1_SPRITE2D_SCALEFLIP   , DLParser_GBI1_Sprite2DScaleFlip );
		//SetCommand( G_GBI1_SPRITE2D_DRAW        , DLParser_GBI1_Sprite2DDraw );
			
	}
	else
	{
		SetCommand( G_GBI2_RDPHALF_2		, DLParser_GBI2_RDPHalf_2 );
		SetCommand( G_GBI2_SETOTHERMODE_H	, DLParser_GBI2_SetOtherModeH );
		SetCommand( G_GBI2_SETOTHERMODE_L	, DLParser_GBI2_SetOtherModeL );
		SetCommand( G_GBI2_RDPHALF_1		, DLParser_GBI2_RDPHalf_1 );
		SetCommand( G_GBI2_SPNOOP			, DLParser_GBI2_SpNoop );
		SetCommand( G_GBI2_ENDDL			, DLParser_GBI2_EndDL );
		SetCommand( G_GBI2_DL				, DLParser_GBI2_DL );
		SetCommand( G_GBI2_LOAD_UCODE		, DLParser_GBI2_LoadUCode );
		SetCommand( G_GBI2_MOVEMEM			, DLParser_GBI2_MoveMem );
		SetCommand( G_GBI2_MOVEWORD			, DLParser_GBI2_MoveWord );
		SetCommand( G_GBI2_MTX				, DLParser_GBI2_Mtx );
		SetCommand( G_GBI2_GEOMETRYMODE		, DLParser_GBI2_GeometryMode );
		SetCommand( G_GBI2_POPMTX			, DLParser_GBI2_PopMtx );
		SetCommand( G_GBI2_TEXTURE			, DLParser_GBI2_Texture );
		SetCommand( G_GBI2_SUBMODULE		, DLParser_GBI2_SubModule );

		SetCommand( G_GBI2_NOOP				, DLParser_GBI2_Noop );
		SetCommand( G_GBI2_VTX				, DLParser_GBI2_Vtx );
		SetCommand( G_GBI2_MODIFYVTX		, DLParser_GBI2_ModifyVtx );
		SetCommand( G_GBI2_CULLDL			, DLParser_GBI2_CullDL );
		SetCommand( G_GBI2_BRANCH_Z			, DLParser_GBI2_BranchZ );
		SetCommand( G_GBI2_TRI1				, DLParser_GBI2_Tri1 );
		SetCommand( G_GBI2_TRI2				, DLParser_GBI2_Tri2 );
		SetCommand( G_GBI2_LINE3D			, DLParser_GBI2_Line3D );
	}

	if ( ucode == 0 )
	{
		gInstructionLookup[G_GBI1_VTX] = DLParser_GBI0_Vtx;			gInstructionName[G_GBI1_VTX]		= "G_GBI0_VTX";
		gInstructionLookup[G_GBI1_TRI2] = DLParser_GBI0_Tri2;		gInstructionName[G_GBI1_TRI2]		= "G_GBI0_TRI2";
	}
	
	if ( ucode == 4 )
	{
		gInstructionLookup[G_GBI1_VTX] = DLParser_GBI0_Vtx_WRUS;	gInstructionName[G_GBI1_VTX]		= "G_GBI0_VTX_WRUS";
	}

	if ( ucode == 6 )
	{
		gInstructionLookup[G_GBI1_VTX] = DLParser_GBI0_Vtx_DKR;		gInstructionName[G_GBI1_VTX]		= "G_GBI0_VTX_DKR";
		gInstructionLookup[G_GBI1_TRI2] = DLParser_GBI0_Tri2;		gInstructionName[G_GBI1_TRI2]		= "G_GBI0_TRI2";

		// DKR
		SetCommand( G_DMATRI,  DLParser_DmaTri );
		SetCommand( G_DLINMEM,  DLParser_DLInMem );
	}



	gUCode = ucode;
}

//*****************************************************************************
//
//*****************************************************************************
#ifndef ARRAYSIZE
#define ARRAYSIZE(x)	(  sizeof( (x)  ) / sizeof(  (x)[0]  )   )
#endif

static u32 DLParser_IdentifyUcodeFromCrc( u32 crc_size, u32 crc_800 )
{
	for ( u32 i = 0; i < ARRAYSIZE(g_MicrocodeData); i++ )
	{
		if ( crc_size == g_MicrocodeData[i].crc_size )
		{
			return g_MicrocodeData[i].ucode;
		}
	}

	return ~0;
}


//*****************************************************************************
// Reads the next command from the display list, updates the PC.
//*****************************************************************************
static bool	DLParser_FetchNextCommand( MicroCodeCommand * p_command )
{
	// Current PC is the last value on the stack
	DList & entry = g_dwPCStack[g_dwPCStack.size() - 1];
	u32 pc = entry.addr;

	if ( pc > MAX_RAM_ADDRESS )
	{
		DBGConsole_Msg(0, "Display list PC is out of range: 0x%08x", pc );
		return false;
	}

	p_command->cmd0 = g_pu32RamBase[(pc>>2)+0];
	p_command->cmd1 = g_pu32RamBase[(pc>>2)+1];

	entry.addr = pc + 8;

	DL_PF("[%05d] 0x%08x: %08x %08x %-10s", gCurrentInstructionCount, pc, p_command->cmd0, p_command->cmd1, gInstructionName[ p_command->cmd ]);

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	gCurrentInstructionCount++;

	if( gInstructionCountLimit != UNLIMITED_INSTRUCTION_COUNT )
	{
		if( gCurrentInstructionCount >= gInstructionCountLimit )
		{
			g_bRDPHalted = true;
		}
	}
#endif

	return true;
}

//*************************************************************************************
// 
//*************************************************************************************
static void HandleDumpDisplayList( OSTask * pTask )
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDumpNextDisplayList)
	{
		DBGConsole_Msg( 0, "Dumping display list" );
		static u32 dwCount = 0;

		char szFilePath[MAX_PATH+1];
		char szFileName[MAX_PATH+1];
		char szDumpDir[MAX_PATH+1];

		IO::Path::Combine(szDumpDir, g_ROM.settings.szGameName, gDisplayListRootPath);
	
		Dump_GetDumpDirectory(szFilePath, szDumpDir);

		sprintf(szFileName, "dl%04d.txt", dwCount++);

		IO::Path::Append(szFilePath, szFileName);

		gDisplayListFile = fopen( szFilePath, "w" );
		if (gDisplayListFile != NULL)
			DBGConsole_Msg(0, "RDP: Dumping Display List as %s", szFilePath);
		else
			DBGConsole_Msg(0, "RDP: Couldn't create dumpfile %s", szFilePath);

		DLParser_DumpTaskInfo( pTask );

		// Clear flag as we're done
		gDumpNextDisplayList = false;
	}
#endif
}

//*****************************************************************************
// 
//*****************************************************************************
static void	DLParser_InitMicrocode( const OSTask * pTask )
{
	u32 base = (u32)pTask->t.ucode_data & 0x1fffffff;
	CHAR str[300] = "";
	for ( u32 i = 0; i < 0x1000; i++ )
	{
		
		if ( g_ps8RamBase[ base + ((i+0) ^ 3) ] == 'R' &&
			 g_ps8RamBase[ base + ((i+1) ^ 3) ] == 'S' &&
			 g_ps8RamBase[ base + ((i+2) ^ 3) ] == 'P' )
		{
			CHAR * p = str;
			while ( g_ps8RamBase[ base + (i ^ 3) ] >= ' ')
			{
				*p++ = g_ps8RamBase[ base + (i ^ 3) ];
				i++;
			}
			*p++ = 0;
			break;
		}
	}

	if ( strcmp( str, gLastMicrocodeString ) != 0 )
	{
		u32 size = pTask->t.ucode_data_size;
		base = (u32)pTask->t.ucode & 0x1fffffff;

		u32 crc_size = daedalus_crc32( 0, &g_pu8RamBase[ base ], size );
		u32 crc_800 = daedalus_crc32( 0, &g_pu8RamBase[ base ], 0x800 );

		//DBGConsole_Msg(0, "Ucode is: 0x%08x, %s", crc_800, str );

		u32 ucode;
		ucode = DLParser_IdentifyUcodeFromCrc( crc_size, crc_800 );
		if ( ucode == u32(~0) )
		{
			CHAR message[300];

			sprintf(message, "Unable to find ucode to use for\n\n"
							  "%s\n"
							  "CRC800: 0x%08x\n"
							  "CRCSize: 0x%08x\n\n"
							  "Please mail StrmnNrmn@emuhelp.com with the contents of c:\\ucodes.txt",
					str, crc_800, crc_size);
			DAEDALUS_ASSERTMSG( message );

			FILE * fh = fopen( "c:\\ucodes.txt", "a" );
			if ( fh )
			{
				fprintf( fh, "%d, 0x%08x, 0x%08x, \"%s\", // %s, \n",
					0xdeadbeef, crc_size, crc_800, str, g_ROM.settings.szGameName );
				fclose(fh);
			}

		}

		DLParser_SetuCode( ucode );
		
		strcpy( gLastMicrocodeString, str );
	}

}


//*****************************************************************************
//	Process the entire display list in one go
//*****************************************************************************

#ifdef DAEDALUS_ENABLE_PROFILING
SProfileItemHandle * gpProfileItemHandles[ 256 ];
#endif

static void	DLParser_ProcessDList()
{
	MicroCodeCommand command;
	g_bRDPHalted = FALSE;
	
	while (!g_bRDPHalted)
	{
		if ( !DLParser_FetchNextCommand( &command ) )
		{
			break;
		}

	#ifdef DAEDALUS_USE_EXCEPTIONS
		try 
		{
			gInstructionLookup[ command.cmd ]( command );
		}

		catch (...)
		{
			DBGConsole_Msg( 0, "Exception processing 0x%08x 0x%08x", command.cmd0, command.cmd1 );
			if (!g_bTrapExceptions)
				throw;
			break;
		}
	#else

			{
#ifdef DAEDALUS_ENABLE_PROFILING
				if(gpProfileItemHandles[ command.cmd ] == NULL)
				{
					gpProfileItemHandles[ command.cmd ] = new SProfileItemHandle( CProfiler::Get()->AddItem( gInstructionName[ command.cmd ] ) );
				}
				CAutoProfile		_auto_profile( *gpProfileItemHandles[ command.cmd ] );
#endif
				gInstructionLookup[ command.cmd ]( command );
			}

	#endif

		// Check limit
		if (!g_bRDPHalted)
		{
			g_dwPCStack[g_dwPCStack.size() - 1].limit--;
			if (g_dwPCStack[g_dwPCStack.size() - 1].limit == u32(~0))
			{
				DL_PF("**EndDLInMem");
				// If we're here, then we musn't have finished with the display list yet
				// Check if this is the last display list in the sequence
				if(g_dwPCStack.size() == 1)
				{
					g_bRDPHalted = TRUE;
					g_dwPCStack.clear();
				}
				else
				{
					g_dwPCStack.pop_back();
				}
			}	
		}
	}

}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_Process()
{
	DAEDALUS_PROFILE( "DLParser_Process" );

	DList dl;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	gTotalInstructionCount = 0;
#endif

	if (!PVRRenderer::IsAvailable() )
	{
		return;
	}

	// Shut down the debug console when we start rendering
	// TODO: Clear the front/backbuffer the first time this function is called
	// to remove any stuff lingering on the screen.
	CDebugConsole::Get()->EnableConsole( false );


	OSTask * pTask = (OSTask *)(g_pu8SpMemBase + 0x0FC0);

	DLParser_InitMicrocode( pTask );

	//
	// Not sure what to init this with. We should probably read it from the dmem
	//
	gRDPOtherMode._u64 = 0;
	gRDPOtherMode.pad = G_RDP_RDPSETOTHERMODE;
	gRDPOtherMode.blender = 0x0050;
	gRDPOtherMode.alpha_compare = 1;

	g_dwOtherModeL = gRDPOtherMode._u32[0];
	g_dwOtherModeH = gRDPOtherMode._u32[1];

	u64 now;
	if(daedalus::NTiming::GetPreciseTime( &now ))
	{
		//
		//	Get tick count in ms
		//
		u64	freq;
		daedalus::NTiming::GetPreciseFrequency( &freq );
		gRDPTime = u32(now * 1000 / freq);
	}
	gRDPFrame++;

	// Check if we need to purge
	//if (gRDPTime - g_dwLastPurgeTimeTime > 5000)
	{
		CTextureCache::Get()->PurgeOldTextures();
		g_dwLastPurgeTimeTime = gRDPTime;
	}

	// Initialise stack
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	gCurrentInstructionCount = 0;
#endif
	g_dwPCStack.clear();
	dl.addr = (u32)pTask->t.data_ptr;
	dl.limit = ~0;
	g_dwPCStack.push_back(dl);

	g_dwNumDListsCulled = 0;
	g_dwNumVertices = 0;

	//
	// Prepare to dump this displaylist, if necessary
	//
	HandleDumpDisplayList( pTask );

	DL_PF("DP: Firing up RDP!");

	// ZBuffer/BackBuffer clearing is caught elsewhere, but we could force it here
	mpRenderer->Reset();


	gTesselatorVerticesIn = 0;
	gTesselatorVerticesOut = 0;

	mpRenderer->BeginScene();

	u32 buffer_width, buffer_height;
    mpRenderer->SetD3DViewport(0, 0, 640, 480);


	tVector3 scale( 640/4.0f, 480/4.0f, 511/4.0f );
	tVector3 trans( 640/4.0f, 480/4.0f, 511/4.0f );


	mpRenderer->SetN64Viewport( scale, trans );

	if(!gSkipNextDisplayList)
	{
		DLParser_ProcessDList();
	}
	else
	{
		gSkipNextDisplayList = false;
	}

	mpRenderer->EndScene();

	// Do this regardless!
	FinishRDPJob();

	// Have image displayed
	// Call RSP BREAK here.
	DL_PF("EndDisplayList: Finished - Calling RSP::BREAK.");

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		fclose( gDisplayListFile );
		gDisplayListFile = NULL;
	}

	gTotalInstructionCount = gCurrentInstructionCount;
	gInstructionCountLimit = UNLIMITED_INSTRUCTION_COUNT;
#endif
}



//*****************************************************************************
//
//*****************************************************************************
void DLParser_PopDL()
{
	DL_PF("      Returning from DisplayList");
	DL_PF("############################################");
	DL_PF("/\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\");
	DL_PF("");

	if (g_dwPCStack.size() == 0)
	{
		DL_PF("EndDisplayList: Too many EndDL calls - ignoring");
		g_bRDPHalted = TRUE;			// Stop, damn it!
		return;
	}

	// Check if this is the lasy display list in the sequence
	if(g_dwPCStack.size() == 1)
	{
		g_bRDPHalted = TRUE;
		g_dwPCStack.clear();
		return;
	}

	// If we're here, then we musn't have finished with the display list yet
	g_dwPCStack.pop_back();

}

//*****************************************************************************
//
//*****************************************************************************
/*
0x002861f8: fd48007f 802999f0 G_RDP_SETTIMG
    Image: 0x002999f0 Fmt: CI/8 Width: 128
0x00286200: f5480800 07000000 G_RDP_SETTILE
    Tile:7  Fmt: CI/8bpp Line:4 TMem:0x0000 Palette:0
         S: Clamp: Off Mirror:Off Mask:0x0 Shift:0x0
         T: Clamp: Off Mirror:Off Mask:0x0 Shift:0x0
0x00286208: e6000000 00000000 G_RDP_RDPLOADSYNC
0x00286210: f407c000 070f807c G_RDP_LOADTILE
    Tile:7 (31,0) -> (62,31) [32 x 32]
    g_TI.Address + dwOffset: 0x00299a0f (LoadTile Pitch: 128)
0x00286218: e7000000 00000000 G_RDP_RDPPIPESYNC
0x00286220: f5480800 00000000 G_RDP_SETTILE
    Tile:0  Fmt: CI/8bpp Line:4 TMem:0x0000 Palette:0
         S: Clamp: Off Mirror:Off Mask:0x0 Shift:0x0
         T: Clamp: Off Mirror:Off Mask:0x0 Shift:0x0
0x00286228: f207c000 000f807c G_RDP_SETTILESIZE
    Tile:0 (31,0) -> (62,31) [32 x 32]



0x00286230: fd48007f 8028d9f0 G_RDP_SETTIMG
    Image: 0x0028d9f0 Fmt: CI/8 Width: 128
0x00286238: f5480880 07000000 G_RDP_SETTILE
    Tile:7  Fmt: CI/8bpp Line:4 TMem:0x0080 Palette:0
         S: Clamp: Off Mirror:Off Mask:0x0 Shift:0x0
         T: Clamp: Off Mirror:Off Mask:0x0 Shift:0x0
0x00286240: e6000000 00000000 G_RDP_RDPLOADSYNC
0x00286248: f407c000 070f807c G_RDP_LOADTILE
    Tile:7 (31,0) -> (62,31) [32 x 32]
    g_TI.Address + dwOffset: 0x0028da0f (LoadTile Pitch: 128)
0x00286250: e7000000 00000000 G_RDP_RDPPIPESYNC
0x00286258: f5480880 01000000 G_RDP_SETTILE
    Tile:1  Fmt: CI/8bpp Line:4 TMem:0x0080 Palette:0
         S: Clamp: Off Mirror:Off Mask:0x0 Shift:0x0
         T: Clamp: Off Mirror:Off Mask:0x0 Shift:0x0
0x00286260: f207c000 010f807c G_RDP_SETTILESIZE
    Tile:1 (31,0) -> (62,31) [32 x 32]



0x00286268: 07343616 0034163c G_GBI2_LINE3D
   Tri: 30,11,26
   Tri: 11,27,26
     *Enabling Texturing for Tile: 0
     *Performing tmap load:
     *  (31,0 -> 63,32)
     *  Pitch: 128, Addr: 0x0028d9f0
     *Enabling Texturing for Tile: 1
     *Performing tmap load:
     *  (31,0 -> 63,32)
     *  Pitch: 128, Addr: 0x0028d9f0
CycleType: 2
      Blend: ONE/ZERO
*/

//*****************************************************************************
//
//*****************************************************************************
void RDP_EnableTexturing( u32 tile_idx )
{
	DAEDALUS_PROFILE( "RDP_EnableTexturing" );

	gRDPStateManager.EnableTexturing( mpRenderer, 0, tile_idx );

#ifdef RDP_USE_TEXEL1
	if ( gRDPOtherMode.text_lod )
	{
		// LOD is enabled - use the highest detail texture in texel1
		gRDPStateManager.EnableTexturing( mpRenderer, 1, tile_idx );
	}
	else
	{
		// LOD is disabled - use two textures
		gRDPStateManager.EnableTexturing( mpRenderer, 1, tile_idx+1 );
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void RDP_MoveMemLight(u32 dwLight, u32 address)
{
	s8 * pcBase = g_ps8RamBase + address;
	u32 * pdwBase = (u32 *)pcBase;

	g_N64Lights[dwLight].Colour     = pdwBase[0];
	g_N64Lights[dwLight].ColourCopy = pdwBase[1];
	g_N64Lights[dwLight].x			= f32(pcBase[8 ^ 0x3]);
	g_N64Lights[dwLight].y			= f32(pcBase[9 ^ 0x3]);
	g_N64Lights[dwLight].z			= f32(pcBase[10 ^ 0x3]);
					
	DL_PF("       RGBA: 0x%08x, RGBACopy: 0x%08x, x: %f, y: %f, z: %f", 
		g_N64Lights[dwLight].Colour,
		g_N64Lights[dwLight].ColourCopy,
		g_N64Lights[dwLight].x,
		g_N64Lights[dwLight].y,
		g_N64Lights[dwLight].z);

	if (dwLight == g_dwAmbientLight)
	{
		DL_PF("      (Ambient Light)");

		u32 n64col( g_N64Lights[dwLight].Colour );

		mpRenderer->SetAmbientLight( N64COL_GETR_F(n64col), N64COL_GETG_F(n64col), N64COL_GETB_F(n64col) );
	}
	else
	{
		
		DL_PF("      (Normal Light)");

		mpRenderer->SetLightCol(dwLight, g_N64Lights[dwLight].Colour);
		if (pdwBase[2] == 0)	// Direction is 0!
		{
			DL_PF("      Light is invalid");
		}
		else
		{
			mpRenderer->SetLightDirection(dwLight, 
										  g_N64Lights[dwLight].x,
										  g_N64Lights[dwLight].y,
										  g_N64Lights[dwLight].z);
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
//0x000b46b0: dc080008 800b46a0 G_GBI2_MOVEMEM
//    Type: 08 Len: 08 Off: 0000
//        Scale: 640 480 511 0 = 160,120
//        Trans: 640 480 511 0 = 160,120
//vscale is the scale applied to the normalized homogeneous coordinates after 4x4 projection transformation 
//vtrans is the offset added to the scaled number

void RDP_MoveMemViewport(u32 address)
{
	s16 scale[4];
	s16 trans[4];

	// address is offset into RD_RAM of 8 x 16bits of data...
	scale[0] = *(s16 *)(g_pu8RamBase + ((address+(0*2))^0x2));
	scale[1] = *(s16 *)(g_pu8RamBase + ((address+(1*2))^0x2));
	scale[2] = *(s16 *)(g_pu8RamBase + ((address+(2*2))^0x2));
	scale[3] = *(s16 *)(g_pu8RamBase + ((address+(3*2))^0x2));

	trans[0] = *(s16 *)(g_pu8RamBase + ((address+(4*2))^0x2));
	trans[1] = *(s16 *)(g_pu8RamBase + ((address+(5*2))^0x2));
	trans[2] = *(s16 *)(g_pu8RamBase + ((address+(6*2))^0x2));
	trans[3] = *(s16 *)(g_pu8RamBase + ((address+(7*2))^0x2));

	// With D3D we had to ensure that the vp coords are positive, so
	// we truncated them to 0. This happens a lot, as things
	// seem to specify the scale as the screen w/2 h/2

	tVector3 vec_scale( scale[0] / 4.0f, scale[1] / 4.0f, scale[2] / 4.0f );
	tVector3 vec_trans( trans[0] / 4.0f, trans[1] / 4.0f, trans[2] / 4.0f );


	mpRenderer->SetN64Viewport( vec_scale, vec_trans );


	DL_PF("        Scale: %d %d %d %d", scale[0], scale[1], scale[2], scale[3]);
	DL_PF("        Trans: %d %d %d %d", trans[0], trans[1], trans[2], trans[3]);
}

//*****************************************************************************
//
//*****************************************************************************
#define RDP_NOIMPL_WARN(op) { DBGConsole_Msg(0, op); }
//#define RDP_NOIMPL_WARN 1 ? (void)0 : (void)
inline void RDP_NOIMPL( LPCTSTR op, u32 cmd0, u32 cmd1 ) 
{
	DBGConsole_Msg( 0, "Not Implemented: %s 0x%08x 0x%08x", op, cmd0, cmd1 );
	DL_PF("~~RDP: Not Implemented: %s", op);

	/*g_bRDPHalted = TRUE;*/
}

//*****************************************************************************
//
//*****************************************************************************
//Nintro64 uses Sprite2d 
void DLParser_Nothing( MicroCodeCommand command )
{
	DBGConsole_Msg( 0, "~~RDP Command %08x Does not exist...", command.cmd0 );
	DL_PF("~~RDP Command Does not exist...");

	// Terminate!
	{
	//	DBGConsole_Msg(0, "Warning, DL cut short with unknown command: 0x%08x 0x%08x", command.cmd0, command.cmd1);

		DLParser_PopDL();


	//	g_bRDPHalted = TRUE;
	//	g_dwPCStack.clear();
	}

}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_SpNoop( MicroCodeCommand command )
{
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_BranchZ( MicroCodeCommand command )
{
	DL_PF( "~*Not Implemented GBI1 BRANCHZ" );

	static bool shown = false;
	if (!shown )
	{
		MessageBox( NULL, "GBI1 BranchZ", "Unknown command", MB_OK );
		shown = true;
	}
}

void DLParser_GBI2_SpNoop( MicroCodeCommand command )
{
	DL_PF( "~*Not Implemented (SPNOOP in GBI 2)" );
	static bool shown = false;
	if (!shown )
	{
		MessageBox( NULL, "SPNOOP", "Unknown command", MB_OK );
		shown = true;
	}
}

void DLParser_GBI2_LoadUCode( MicroCodeCommand command )
{
	DL_PF( "~*G_LOAD_UCODE!" );
	static bool shown = false;
	if (!shown )
	{
		MessageBox( NULL, "G_LOAD_UCODE", "Unknown command", MB_OK );
		shown = true;
	}
}

void DLParser_GBI2_BranchZ( MicroCodeCommand command )
{
	DL_PF( "~*Not Implemented (G_BRANCH_Z in GBI 2)" );
	static bool shown = false;
	if (!shown )
	{
		MessageBox( NULL, "G_BRANCH_Z", "Unknown command", MB_OK );
		shown = true;
	}
}

void DLParser_GBI2_SubModule( MicroCodeCommand command )
{
	DL_PF( "~*Not Implemented (G_SUB_MODULE in GBI 2)" );
	static bool shown = false;
	if (!shown )
	{
		MessageBox( NULL, "G_SUB_MODULE", "Unknown command", MB_OK );
		shown = true;
	}
}


void DLParser_GBI2_Noop( MicroCodeCommand command )
{
}


void DLParser_GBI2_RDPHalf_1( MicroCodeCommand command )
{

}
void DLParser_GBI2_RDPHalf_2( MicroCodeCommand command )
{

}

//*****************************************************************************
//
//*****************************************************************************
// AST uses this
void DLParser_GBI1_LoadUCode( MicroCodeCommand command )
{
	DL_PF( "~*Not Implemented - GBI1 LoadUCode" );
	// 
	static BOOL bWarned = FALSE;
	if (!bWarned)
	{
		RDP_NOIMPL("RDP: DLParser_GBI1_LoadUCode (0x%08x 0x%08x)", command.cmd0, command.cmd1);
		bWarned = TRUE;
	}

}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Reserved0( MicroCodeCommand command )
{	
	DL_PF( "~*Not Implemented" );

	// Spiderman
	static BOOL bWarned = FALSE;

	if (!bWarned)
	{
		RDP_NOIMPL("RDP: Reserved1 (0x%08x 0x%08x)", command.cmd0, command.cmd1);
		bWarned = TRUE;
	}
}
//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Reserved1( MicroCodeCommand command )
{	
	DL_PF( "~*Not Implemented" );

	// Spiderman
	static BOOL bWarned = FALSE;

	if (!bWarned)
	{
		RDP_NOIMPL("RDP: Reserved1 (0x%08x 0x%08x)", command.cmd0, command.cmd1);
		bWarned = TRUE;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Reserved2( MicroCodeCommand command )
{	
	DL_PF( "~*Not Implemented" );

	// Spiderman
	static BOOL bWarned = FALSE;

	if (!bWarned)
	{
		RDP_NOIMPL("RDP: Reserved2 (0x%08x 0x%08x)", command.cmd0, command.cmd1);
		bWarned = TRUE;
	}
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Reserved3( MicroCodeCommand command )
{	
	DL_PF( "~*Not Implemented" );

	// Spiderman
	static BOOL bWarned = FALSE;

	if (!bWarned)
	{
		RDP_NOIMPL("RDP: Reserved3 (0x%08x 0x%08x)", command.cmd0, command.cmd1);
		bWarned = TRUE;
	}
}

//*****************************************************************************
//
//0xb2140002 0x076d06eb
//*****************************************************************************
void DLParser_GBI1_ModifyVtx( MicroCodeCommand command )
{
	u32 offset =  (command.cmd0 >> 16) & 0xFF;
	u32 vert   = ((command.cmd0      ) & 0xFFFF) / gVertexStride;			// or just 2?
	u32 value  = command.cmd1;

	switch ( offset )
	{
		case G_MWO_POINT_RGBA:
			{
				DL_PF("      Setting RGBA to 0x%08x", value);
				mpRenderer->SetVtxColor( vert, value );
			}
			break;

		case G_MWO_POINT_ST:
			{
				s16 tu = s16(value>>16);
				s16 tv = s16(value & 0xFFFF);
				DL_PF( "      Setting tu/tv to %f, %f", tu/32.0f, tv/32.0f );
				mpRenderer->SetVtxTextureCoord( vert, tu, tv );
			}
			break;

		case G_MWO_POINT_XYSCREEN:
			{
				u16 x = u16(value>>16);
				u16 y = u16(value & 0xFFFF);
				DL_PF( "      Setting XY Screen to 0x%08x (%d,%d)", value, x, y );
				mpRenderer->SetVtxXY( vert, x / 4.0f, y / 4.0f );
			}
			break;

		case G_MWO_POINT_ZSCREEN:
			{
				DL_PF( "      Setting ZScreen to 0x%08x", value );
			}
			break;

		default:
			DBGConsole_Msg( 0, "ModifyVtx - Setting vert data 0x%02x, 0x%08x", offset, value );
			DL_PF( "      Setting unk value: 0x%02x, 0x%08x", offset, value );
			break;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_ModifyVtx( MicroCodeCommand command )
{
	u32 offset =  (command.cmd0 >> 16) & 0xFF;
	u32 vert   = ((command.cmd0      ) & 0xFFFF) / 2;
	u32 value  = command.cmd1;

	switch ( offset )
	{
		case G_MWO_POINT_RGBA:
			{
				DL_PF("      Setting RGBA to 0x%08x", value);
				mpRenderer->SetVtxColor( vert, value );
			}
			break;

		case G_MWO_POINT_ST:
			{
				s16 tu = s16(value>>16);
				s16 tv = s16(value & 0xFFFF);
				DL_PF( "      Setting tu/tv to %f, %f", tu/32.0f, tv/32.0f );
				mpRenderer->SetVtxTextureCoord( vert, tu, tv );
			}
			break;

		case G_MWO_POINT_XYSCREEN:
			{
				u16 x = u16(value>>16);
				u16 y = u16(value & 0xFFFF);
				DL_PF( "      Setting XY Screen to 0x%08x (%d,%d)", value, x, y );
				mpRenderer->SetVtxXY( vert, x / 4.0f, y / 4.0f );
			}
			break;

		case G_MWO_POINT_ZSCREEN:
			{
				DL_PF( "      Setting ZScreen to 0x%08x", value );
			}
			break;

		default:
			DBGConsole_Msg( 0, "ModifyVtx - Setting vert data 0x%02x, 0x%08x", offset, value );
			DL_PF( "      Setting unk value: 0x%02x, 0x%08x", offset, value );
			break;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Sprite2DBase( MicroCodeCommand command )
{
	DL_PF( "~*Not Implemented" );
	u32 address = RDPSegAddr(command.cmd1);

	use(address);

	DL_PF("    Address:0x%08x", address);
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Sprite2DScaleFlip( MicroCodeCommand command )
{
	DL_PF( "~*Not Implemented" );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Sprite2DDraw( MicroCodeCommand command )
{
	DL_PF( "~*Not Implemented" );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Noop( MicroCodeCommand command )
{
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetKeyGB( MicroCodeCommand command )
{
	static BOOL bWarned = FALSE;

	DL_PF("    SetKeyGB (not implemented)");
	if (!bWarned)
	{
		RDP_NOIMPL("RDP: SetKeyGB 0x%08x 0x%08x", command.cmd0, command.cmd1);
		bWarned = TRUE;
	}

}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetKeyR( MicroCodeCommand command )
{
	static BOOL bWarned = FALSE;

	DL_PF("    SetKeyR (not implemented)");
	if (!bWarned)
	{
		RDP_NOIMPL("RDP: SetKeyR 0x%08x 0x%08x", command.cmd0, command.cmd1);
		bWarned = TRUE;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetConvert( MicroCodeCommand command )
{
	
	// Spiderman
	static BOOL bWarned = FALSE;

	DL_PF("    SetConvert (not implemented)");
	if (!bWarned)
	{
		RDP_NOIMPL("RDP: SetConvert 0x%08x 0x%08x", command.cmd0, command.cmd1);
		bWarned = TRUE;
	}
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetPrimDepth( MicroCodeCommand command )
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	u32 dwZ  = (command.cmd1 >> 16) & 0xFFFF;
	u32 dwDZ = (command.cmd1      ) & 0xFFFF;

	DL_PF("SetPrimDepth: 0x%08x 0x%08x - z: 0x%04x dz: 0x%04x", command.cmd0, command.cmd1, dwZ, dwDZ);
#endif	
	// Not implemented!
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_RDPSetOtherMode( MicroCodeCommand command )
{
	DL_PF( "      RDPSetOtherMode: 0x%08x 0x%08x", command.cmd0, command.cmd1 );

	g_dwOtherModeH = command.cmd0;
	g_dwOtherModeL = command.cmd1;

	RDP_SetOtherMode( command.cmd0, command.cmd1 );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_RDPHalf_Cont( MicroCodeCommand command )
{
	//DBGConsole_Msg( 0, "Unexpected RDPHalf_Cont: %08x %08x", command.cmd0, command.cmd1 );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_RDPHalf_2( MicroCodeCommand command )
{
	//DBGConsole_Msg( 0, "Unexpected RDPHalf_2: %08x %08x", command.cmd0, command.cmd1 );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_RDPHalf_1( MicroCodeCommand command )
{
	//DBGConsole_Msg( 0, "Unexpected RDPHalf_1: %08x %08x", command.cmd0, command.cmd1 );
}




//*****************************************************************************
//
//*****************************************************************************
void DLParser_RDPLoadSync( MicroCodeCommand command )	{ /*DL_PF("LoadSync: (Ignored)");*/ }
void DLParser_RDPPipeSync( MicroCodeCommand command )	{ /*DL_PF("PipeSync: (Ignored)");*/ }
void DLParser_RDPTileSync( MicroCodeCommand command )	{ /*DL_PF("TileSync: (Ignored)");*/ }

//*****************************************************************************
//
//*****************************************************************************
void DLParser_RDPFullSync( MicroCodeCommand command )
{ 
	// We now do this regardless
	FinishRDPJob();

	/*DL_PF("FullSync: (Generating Interrupt)");*/
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_DL( MicroCodeCommand command )
{	
	DList dl;
	u32 dwPush = (command.cmd0 >> 16) & 0xFF;
	u32 dwAddr = RDPSegAddr(command.cmd1);

	DL_PF("    Address=0x%08x Push: 0x%02x", dwAddr, dwPush);
	
	switch (dwPush)
	{
	case G_DL_PUSH:
		DL_PF("    Pushing DisplayList 0x%08x", dwAddr);
		dl.addr = dwAddr;
		dl.limit = ~0;
		g_dwPCStack.push_back( dl );

		break;
	case G_DL_NOPUSH:
		DL_PF("    Jumping to DisplayList 0x%08x", dwAddr);
		g_dwPCStack[g_dwPCStack.size() - 1].addr = dwAddr;
		g_dwPCStack[g_dwPCStack.size() - 1].limit = ~0;
		break;
	}

	DL_PF("");
	DL_PF("\\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/");
	DL_PF("############################################");
}

//*****************************************************************************
//
//*****************************************************************************
// BB2k
// DKR
//00229B70: 07020010 000DEFC8 CMD G_DLINMEM  Displaylist at 800DEFC8 (stackp 1, limit 2)
//00229A58: 06000000 800DE520 CMD G_GBI1_DL  Displaylist at 800DE520 (stackp 1, limit 0)
//00229B90: 07070038 00225850 CMD G_DLINMEM  Displaylist at 80225850 (stackp 1, limit 7)

/* flags to inhibit pushing of the display list (on branch) */
//#define G_DL_PUSH		0x00
//#define G_DL_NOPUSH		0x01


void DLParser_DLInMem( MicroCodeCommand command )
{
	DList dl;
	//DBGConsole_Msg(0, "DLInMem: 0x%08x 0x%08x", command.cmd0, command.cmd1);

	u32 dwLimit = (command.cmd0 >> 16) & 0xFF;
	u32 dwPush = G_DL_PUSH; //(command.cmd0 >> 16) & 0xFF;
	u32 dwAddr = 0x00000000 | command.cmd1; //RDPSegAddr(command.cmd1);

	DL_PF("    Address=0x%08x Push: 0x%02x", dwAddr, dwPush);
	
	switch (dwPush)
	{
	case G_DL_PUSH:
		DL_PF("    Pushing DisplayList 0x%08x", dwAddr);
		dl.addr = dwAddr;
		dl.limit = dwLimit;
		g_dwPCStack.push_back( dl );

		break;
	case G_DL_NOPUSH:
		DL_PF("    Jumping to DisplayList 0x%08x", dwAddr);
		g_dwPCStack[g_dwPCStack.size() - 1].addr = dwAddr;
		g_dwPCStack[g_dwPCStack.size() - 1].limit = dwLimit;
		break;
	}

	DL_PF("");
	DL_PF("\\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/");
	DL_PF("#############################################");


}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_DL(  MicroCodeCommand command  )
{
	DList dl;

	u32 push = (command.cmd0 >> 16) & 0x01;
	u32 address = RDPSegAddr(command.cmd1);

	DL_PF("    Push:0x%02x Addr: 0x%08x", push, address);
	
	switch ( push )
	{
	case G_DL_PUSH:
		DL_PF("    Pushing ZeldaDisplayList 0x%08x", address);
		dl.addr = address;
		dl.limit = ~0;
		g_dwPCStack.push_back( dl );

		break;
	case G_DL_NOPUSH:
		DL_PF("    Jumping to ZeldaDisplayList 0x%08x", address);
		g_dwPCStack[g_dwPCStack.size() - 1].addr = address;
		g_dwPCStack[g_dwPCStack.size() - 1].limit = ~0;
		break;
	}

	if ( g_dwPCStack.size() > 30 )
	{
		// Stack is too deep - probably an error
		DAEDALUS_ASSERTMSG( "Stack is over 30 - something is probably wrong\n" );
	}

	DL_PF("");
	DL_PF("\\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/");
	DL_PF("#############################################");
}



//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_EndDL( MicroCodeCommand command )
{
	DLParser_PopDL();
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_EndDL( MicroCodeCommand command )
{
	DLParser_PopDL();
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_CullDL( MicroCodeCommand command )
{
	return;

	u32 i;
	u32 dwVFirst = ((command.cmd0) & 0xFFF) / gVertexStride;
	u32 dwVLast  = ((command.cmd1) & 0xFFF) / gVertexStride;

	DL_PF("    Culling using verts %d to %d", dwVFirst, dwVLast);

	// Mask into range
	dwVFirst &= 0x1f;
	dwVLast &= 0x1f;

	for (i = dwVFirst; i <= dwVLast; i++)
	{
		if (mpRenderer->GetVtxFlags( i ) == 0)
		{
			DL_PF("    Vertex %d is visible, continuing with display list processing", i);
			return;
		}
	}

	g_dwNumDListsCulled++;

	DL_PF("    No vertices were visible, culling rest of display list");

	DLParser_PopDL();

}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_CullDL( MicroCodeCommand command )
{
	u32 dwVFirst = ((command.cmd0) & 0xfff) / 2;
	u32 dwVLast  = ((command.cmd1) & 0xfff) / 2;

	DL_PF("    Culling using verts %d to %d", dwVFirst, dwVLast);

	if ( mpRenderer->TestVerts( dwVFirst, dwVLast ) )
	{
		DL_PF( "    Display list is visible, returning" );
	}
	else
	{
		g_dwNumDListsCulled++;
		DL_PF("    Display list is invisible, culling");

		DLParser_PopDL();
	}
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_MoveWord( MicroCodeCommand command )
{
	// Type of movement is in low 8bits of cmd0.

	u32 dwIndex = command.cmd0 & 0xFF;
	u32 dwOffset = (command.cmd0 >> 8) & 0xFFFF;

	switch (dwIndex)
	{
	case G_MW_MATRIX:
		DL_PF("    G_MW_MATRIX");
		RDP_NOIMPL_WARN("G_MW_MATRIX Not Implemented");

		break;
	case G_MW_NUMLIGHT:
		//#define NUML(n)		(((n)+1)*32 + 0x80000000)
		if (gUCode != 6)
		{
			u32 dwNumLights = ((command.cmd1-0x80000000)/32) - 1;
			DL_PF("    G_MW_NUMLIGHT: Val:%d", dwNumLights);

			g_dwAmbientLight = dwNumLights;
			mpRenderer->SetNumLights(dwNumLights);
		}
		else
		{
			// DKR
			u32 dwNumLights = command.cmd1;
			DL_PF("    G_MW_NUMLIGHT: Val:%d", dwNumLights);

			g_dwAmbientLight = dwNumLights;
			mpRenderer->SetNumLights(dwNumLights);

		}
		break;
	case G_MW_CLIP:
		{
			switch (dwOffset)
			{
			case G_MWO_CLIP_RNX:		DL_PF("    G_MW_CLIP  NegX: %d", (s32)(s16)command.cmd1);			break;
			case G_MWO_CLIP_RNY:		DL_PF("    G_MW_CLIP  NegY: %d", (s32)(s16)command.cmd1);			break;
			case G_MWO_CLIP_RPX:		DL_PF("    G_MW_CLIP  PosX: %d", (s32)(s16)command.cmd1);			break;
			case G_MWO_CLIP_RPY:		DL_PF("    G_MW_CLIP  PosY: %d", (s32)(s16)command.cmd1);			break;
			default:					DL_PF("    G_MW_CLIP  ?   : 0x%08x", command.cmd1);					break;
			}

			//RDP_NOIMPL_WARN("G_MW_CLIP Not Implemented");
		}
		break;
	case G_MW_SEGMENT:
		{
			u32 dwSegment = (dwOffset >> 2) & 0xF;
			u32 dwBase = command.cmd1;
			DL_PF("    G_MW_SEGMENT Seg[%d] = 0x%08x", dwSegment, dwBase);
			g_dwSegment[dwSegment] = dwBase;
		}
		break;
	case G_MW_FOG:
		{
			u16 wMult = u16(command.cmd1 >> 16);
			u16 wOff  = u16(command.cmd1      );

			f32 fMult = ((f32)(s16)wMult) / 65536.0f;
			f32 fOff  = ((f32)(s16)wOff)  / 65536.0f;

			DL_PF("     G_MW_FOG. Mult = 0x%04x (%f), Off = 0x%04x (%f)", wMult, wOff, 255.0f * fMult, 255.0f * fOff );

			mpRenderer->SetFogMult( 255.0f * fMult );
			mpRenderer->SetFogOffset( 255.0f * fOff );
		}
		//RDP_NOIMPL_WARN("G_MW_FOG Not Implemented");
		break;
	case G_MW_LIGHTCOL:
		if (gUCode != 6)
		{
			u32 dwLight = dwOffset / 0x20;
			u32 dwField = (dwOffset & 0x7);

			DL_PF("    G_MW_LIGHTCOL/0x%08x: 0x%08x", dwOffset, command.cmd1);

			switch (dwField)
			{
			case 0:
				//g_N64Lights[dwLight].Colour = command.cmd1;
				// Light col, not the copy
				if (dwLight == g_dwAmbientLight)
				{
					u32 n64col( command.cmd1 );

					mpRenderer->SetAmbientLight( N64COL_GETR_F(n64col), N64COL_GETG_F(n64col), N64COL_GETB_F(n64col) );
				}
				else
				{
					mpRenderer->SetLightCol(dwLight, command.cmd1);
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
			mpRenderer->ResetMatrices();
		}

		break;
	case G_MW_POINTS:
		DL_PF("    G_MW_POINTS");
		RDP_NOIMPL_WARN("G_MW_POINTS Not Implemented");
		break;
	case G_MW_PERSPNORM:
		DL_PF("    G_MW_PERSPNORM");
		//RDP_NOIMPL_WARN("G_MW_PESPNORM Not Implemented");		// Used in Starfox - sets to 0xa
	//	if ((short)command.cmd1 != 10)
	//		DBGConsole_Msg(0, "PerspNorm: 0x%04x", (short)command.cmd1);	
		break;
	default:
		DL_PF("    Type: Unknown");
		RDP_NOIMPL_WARN("Unknown MoveWord");
		break;
	}

}


//*****************************************************************************
//
//*****************************************************************************
//0016A710: DB020000 00000018 CMD Zelda_MOVEWORD  Mem[2][00]=00000018 Lightnum=0
//001889F0: DB020000 00000030 CMD Zelda_MOVEWORD  Mem[2][00]=00000030 Lightnum=2
void DLParser_GBI2_MoveWord( MicroCodeCommand command )
{
	u32 dwType   = (command.cmd0 >> 16) & 0xFF;
	u32 dwOffset = (command.cmd0      ) & 0xFFFF;

# define G_MW_FORCEMTX		0x0c
/*
#define G_MW_MATRIX			0x00
#define G_MW_NUMLIGHT		0x02
#define G_MW_CLIP			0x04
#define G_MW_SEGMENT		0x06
#define G_MW_FOG			0x08
#define G_MW_LIGHTCOL		0x0a
#ifdef	F3DEX_GBI_2x
# define G_MW_FORCEMTX		0x0c
#else	// F3DEX_GBI_2
# define G_MW_POINTS		0x0c
#endif	// F3DEX_GBI_2
#define	G_MW_PERSPNORM		0x0e
*/

	switch (dwType)
	{
	case G_MW_MATRIX:
		{
			RDP_NOIMPL_WARN( "G_MW_MATRIX not implemented" );
		}
		break;
	case G_MW_NUMLIGHT:
		{
			// Lightnum
			// command.cmd1:
			// 0x18 = 24 = 0 lights
			// 0x30 = 48 = 2 lights

			u32 dwNumLights = (command.cmd1/12) - 2;
			DL_PF("     G_MW_NUMLIGHT: %d", dwNumLights);

			g_dwAmbientLight = dwNumLights;
			mpRenderer->SetNumLights(dwNumLights);
		}
		break;

	case G_MW_CLIP:
		{
			switch (dwOffset)
			{
			case G_MWO_CLIP_RNX:		DL_PF("     G_MW_CLIP  NegX: %d", (s32)(s16)command.cmd1);			break;
			case G_MWO_CLIP_RNY:		DL_PF("     G_MW_CLIP  NegY: %d", (s32)(s16)command.cmd1);			break;
			case G_MWO_CLIP_RPX:		DL_PF("     G_MW_CLIP  PosX: %d", (s32)(s16)command.cmd1);			break;
			case G_MWO_CLIP_RPY:		DL_PF("     G_MW_CLIP  PosY: %d", (s32)(s16)command.cmd1);			break;
			default:					DL_PF("     G_MW_CLIP");											break;
			}
			//RDP_NOIMPL_WARN("G_MW_CLIP Not Implemented");
		}
		break;

	case G_MW_SEGMENT:
		{
			u32 dwSeg   = dwOffset / 4;
			u32 address	= command.cmd1;

			DL_PF( "      G_MW_SEGMENT Segment[%d] = 0x%08x", dwSeg, address );

			g_dwSegment[dwSeg] = address;
		}
		break;
	case G_MW_FOG:
		{
			u16 wMult = u16(command.cmd1 >> 16);
			u16 wOff  = u16(command.cmd1      );

			f32 fMult = ((f32)(s16)wMult) / 65536.0f;
			f32 fOff  = ((f32)(s16)wOff)  / 65536.0f;

			DL_PF("     G_MW_FOG. Mult = 0x%04x (%f), Off = 0x%04x (%f)", wMult, wOff, 255.0f * fMult, 255.0f * fOff );

			mpRenderer->SetFogMult( 255.0f * fMult );
			mpRenderer->SetFogOffset( 255.0f * fOff );
		}
		break;
	case G_MW_LIGHTCOL:
		{
			u32 dwLight = dwOffset / 0x18;
			u32 dwField = (dwOffset & 0x7);

			DL_PF("     G_MW_LIGHTCOL/0x%08x: 0x%08x", dwOffset, command.cmd1);

			switch (dwField)
			{
			case 0:
				//g_N64Lights[dwLight].Colour = command.cmd1;
				// Light col, not the copy
				if (dwLight == g_dwAmbientLight)
				{
					u32 n64col( command.cmd1 );

					mpRenderer->SetAmbientLight( N64COL_GETR_F(n64col), N64COL_GETG_F(n64col), N64COL_GETB_F(n64col) );
				}
				else
				{
					mpRenderer->SetLightCol(dwLight, command.cmd1);
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

	case G_MW_FORCEMTX:
		RDP_NOIMPL_WARN( "G_MW_FORCEMTX not implemented" );
		break;

	case G_MW_PERSPNORM:
		DL_PF("     G_MW_PERSPNORM 0x%04x", (s16)command.cmd1);
		break;

	default:
		{
			DL_PF("      Ignored!!");

		}
		break;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_MoveMem( MicroCodeCommand command )
{
	u32 dwType    = (command.cmd0>>16)&0xFF;
	u32 dwLength  = (command.cmd0)&0xFFFF;
	u32 address   = RDPSegAddr(command.cmd1);

	use(dwLength);

	switch (dwType)
	{
		case G_MV_VIEWPORT:
			{
				DL_PF("    G_MV_VIEWPORT. Address: 0x%08x, Length: 0x%04x", address, dwLength);
				RDP_MoveMemViewport( address );
			}
			break;
		case G_MV_LOOKATY:
			DL_PF("    G_MV_LOOKATY");
			//RDP_NOIMPL_WARN("G_MV_LOOKATY Not Implemented");
			break;
		case G_MV_LOOKATX:
			DL_PF("    G_MV_LOOKATX");
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
				DL_PF("    G_MV_L%d", dwLight);

				// Ensure dwType == G_MV_L0 -- G_MV_L7
				//	if (dwType < G_MV_L0 || dwType > G_MV_L7 || ((dwType & 0x1) != 0))
				//		break;

				DL_PF("    Light%d: Length:0x%04x, Address: 0x%08x", dwLight, dwLength, address);

				RDP_MoveMemLight(dwLight, address);
			}
			break;
		case G_MV_TXTATT:
			DL_PF("    G_MV_TXTATT");
			RDP_NOIMPL_WARN("G_MV_TXTATT Not Implemented");
			break;
		case G_MV_MATRIX_1:
			DL_PF("    G_MV_MATRIX_1");
			RDP_NOIMPL_WARN("G_MV_MATRIX_1 Not Implemented");
			break;
		case G_MV_MATRIX_2:
			DL_PF("    G_MV_MATRIX_2");
			RDP_NOIMPL_WARN("G_MV_MATRIX_2 Not Implemented");
			break;
		case G_MV_MATRIX_3:
			DL_PF("    G_MV_MATRIX_3");
			RDP_NOIMPL_WARN("G_MV_MATRIX_3 Not Implemented");
			break;
		case G_MV_MATRIX_4:
			DL_PF("    G_MV_MATRIX_4");
			RDP_NOIMPL_WARN("G_MV_MATRIX_4 Not Implemented");
			break;
		default:
			DL_PF("    Type: Unknown");
			{
				static BOOL bWarned = FALSE;
				if (!bWarned)
				{
					DBGConsole_Msg(0, "Unknown Move Type: %d", dwType);
					bWarned = TRUE;
				}
			}
			break;
	}
}


//*****************************************************************************
//
//*****************************************************************************
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

# define G_GBI1_MV_VIEWPORT	0x80
# define G_GBI1_MV_LOOKATY	0x82
# define G_GBI1_MV_LOOKATX	0x84
# define G_GBI1_MV_L0	0x86
# define G_GBI1_MV_L1	0x88
# define G_GBI1_MV_L2	0x8a
# define G_GBI1_MV_L3	0x8c
# define G_GBI1_MV_L4	0x8e
# define G_GBI1_MV_L5	0x90
# define G_GBI1_MV_L6	0x92
# define G_GBI1_MV_L7	0x94
# define G_GBI1_MV_TXTATT	0x96
# define G_GBI1_MV_MATRIX_1	0x9e	// NOTE: this is in moveword table
# define G_GBI1_MV_MATRIX_2	0x98
# define G_GBI1_MV_MATRIX_3	0x9a
# define G_GBI1_MV_MATRIX_4	0x9c

// 0,2,4,6 are reserved by G_MTX
# define G_GBI2_MV_VIEWPORT	8
# define G_GBI2_MV_LIGHT	10
# define G_GBI2_MV_POINT	12
# define G_GBI2_MV_MATRIX	14		// NOTE: this is in moveword table
# define G_GBI2_MVO_LOOKATX	(0*24)
# define G_GBI2_MVO_LOOKATY	(1*24)
# define G_GBI2_MVO_L0	(2*24)
# define G_GBI2_MVO_L1	(3*24)
# define G_GBI2_MVO_L2	(4*24)
# define G_GBI2_MVO_L3	(5*24)
# define G_GBI2_MVO_L4	(6*24)
# define G_GBI2_MVO_L5	(7*24)
# define G_GBI2_MVO_L6	(8*24)
# define G_GBI2_MVO_L7	(9*24)

void DLParser_GBI2_MoveMem( MicroCodeCommand command )
{

/*
#define	gsDma1p(c, s, l, p)									\
{															\
	(_SHIFTL((c), 24, 8) | _SHIFTL((p), 16, 8) | 			\
	 _SHIFTL((l), 0, 16)), 									\
        (unsigned int)(s)									\
}
#define	gsDma2p(c, adrs, len, idx, ofs)						\
{															\
	(_SHIFTL((c),24,8)|_SHIFTL((ofs)/8,16,8)|				\
	 _SHIFTL(((len)-1)/8,8,8)|_SHIFTL((idx),0,8)),			\
        (unsigned int)(adrs)								\
}
*/

	u32 address	   = RDPSegAddr(command.cmd1);
	//u32 dwOffset = (command.cmd0 >> 8) & 0xFFFF;
	u32 dwType	   = (command.cmd0     ) & 0xFE;

	u32 dwLen = (command.cmd0 >> 16) & 0xFF;
	u32 dwOffset2 = (command.cmd0 >> 5) & 0x3FFF;

	use(dwLen);

	DL_PF("    Type: %02x Len: %02x Off: %04x", dwType, dwLen, dwOffset2);

	//ZeldaMoveMem: Unknown Type. 0xdc08030a 0x80063160
	// offset = 0803
	// type = 0a

	s8 * pcBase = g_ps8RamBase + address;

	use(pcBase);

	BOOL bHandled = FALSE;

	switch (dwType)
	{
	case G_GBI2_MV_VIEWPORT:
		{
			RDP_MoveMemViewport( address );
			bHandled = TRUE;
		}
		break;
	case G_GBI2_MV_LIGHT:
		{
		switch (dwOffset2)
		{
		case 0x00:
			{
				DL_PF("    G_MV_LOOKATX %f %f %f", f32(pcBase[8 ^ 0x3]), f32(pcBase[9 ^ 0x3]), f32(pcBase[10 ^ 0x3]));
				bHandled = TRUE;
			}
			break;
		case 0x18:
			{
				DL_PF("    G_MV_LOOKATY %f %f %f", f32(pcBase[8 ^ 0x3]), f32(pcBase[9 ^ 0x3]), f32(pcBase[10 ^ 0x3]));
				bHandled = TRUE;
			}
			break;
		default:		//0x30/48/60
			{
				u32 dwLight = (dwOffset2 - 0x30)/0x18;
				DL_PF("    Light %d:", dwLight);
				RDP_MoveMemLight(dwLight, address);
				bHandled = TRUE;
			}
			break;
		}
		break;

		}
	}

	if (!bHandled)
	{
		DBGConsole_Msg(0, "GBI2 MoveMem: Unknown Type. 0x%08x 0x%08x", command.cmd0, command.cmd1);
		u32 * pdwBase = (u32 *)pcBase;
		
		for (u32 i = 0; i < 4; i++)
		{
			DL_PF("    %08x %08x %08x %08x", pdwBase[0], pdwBase[1], pdwBase[2], pdwBase[3]);
			pdwBase+=4;
		}
	}

}

void DLParser_GBI1_Mtx( MicroCodeCommand command )
{	
	u32 address   = RDPSegAddr(command.cmd1);
	u32 dwCommand = (command.cmd0>>16)&0xFF;
	u32 dwLength  = (command.cmd0)    &0xFFFF;

	use(dwLength);

	DL_PF("    Command: %s %s %s Length %d Address 0x%08x",
		(dwCommand & G_MTX_PROJECTION) ? "Projection" : "ModelView",
		(dwCommand & G_MTX_LOAD) ? "Load" : "Mul",	
		(dwCommand & G_MTX_PUSH) ? "Push" : "NoPush",
		dwLength, address);

	if (address + 64 > MAX_RAM_ADDRESS)
	{
		DBGConsole_Msg(0, "Mtx: Address invalid (0x%08x)", address);
		return;
	}

	// Load matrix from address
	tMatrix mat;
	u32 dwI;
	u32 dwJ;
	const f32 fRecip = 1.0f / 65536.0f;
	
	for (dwI = 0; dwI < 4; dwI++)
	{
		for (dwJ = 0; dwJ < 4; dwJ++)
		{
			s16 hi = *(s16 *)(g_pu8RamBase + ((address+(dwI<<3)+(dwJ<<1)     )^0x2));
			u16 lo = *(u16 *)(g_pu8RamBase + ((address+(dwI<<3)+(dwJ<<1) + 32)^0x2));

			mat.m[dwI][dwJ] = ((hi<<16) | lo ) * fRecip;
		}
	}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		DL_PF(
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n",
			mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
			mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
			mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
			mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
	}
#endif

	PVRRenderer::EMatrixLoadStyle nLoadCommand = dwCommand & G_MTX_LOAD ? PVRRenderer::MATRIX_LOAD : PVRRenderer::MATRIX_MUL;
	BOOL bPush = dwCommand & G_MTX_PUSH ? TRUE : FALSE;


	if (dwCommand & G_MTX_PROJECTION)
	{
		// So far only Extreme-G seems to Push/Pop projection matrices	
		mpRenderer->SetProjection(mat, bPush, nLoadCommand);
	}
	else
	{
		mpRenderer->SetWorldView(mat, bPush, nLoadCommand);
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


static void DLParser_MtxDKR( MicroCodeCommand command )
{	
	u32 address = RDPSegAddr(command.cmd1);
	u32 dwCommand = (command.cmd0>>16)&0xFF;
	u32 dwLength  = (command.cmd0)    &0xFFFF;

	use(dwLength);

	PVRRenderer::EMatrixLoadStyle nLoadCommand = dwCommand & G_MTX_LOAD ? PVRRenderer::MATRIX_LOAD : PVRRenderer::MATRIX_MUL;
	BOOL bPush = dwCommand & G_MTX_PUSH ? TRUE : FALSE;
	
	if (dwCommand == 0)
	{
	//	mpRenderer->ResetMatrices();
		nLoadCommand = PVRRenderer::MATRIX_LOAD;
	}

	if (dwCommand & 0x80)
	{
		nLoadCommand = PVRRenderer::MATRIX_MUL;
	}
	else
	{	
		nLoadCommand = PVRRenderer::MATRIX_LOAD;
	}
		nLoadCommand = PVRRenderer::MATRIX_LOAD;
//00229B00: BC000008 64009867 CMD G_MOVEWORD  Mem[8][00]=64009867 Fogrange 0.203..0.984 zmax=0.000000
//0x0021eef0: bc000008 64009867 G_MOVEWORD
	
/*
	if (dwCommand & 0x40)
	{*/
		bPush = FALSE;
/*	}
	else
	{
		bPush = TRUE;
	}*/
	DL_PF("    Command: %s %s %s Length %d Address 0x%08x",
		(dwCommand & G_MTX_PROJECTION) ? "Projection" : "ModelView",
		(dwCommand & G_MTX_LOAD) ? "Load" : "Mul",	
		(dwCommand & G_MTX_PUSH) ? "Push" : "NoPush",
		dwLength, address);

	if (address + 64 > MAX_RAM_ADDRESS)
	{
		DBGConsole_Msg(0, "Mtx: Address invalid (0x%08x)", address);
		return;
	}

	// Load matrix from address
	tMatrix mat;
	u32 dwI;
	u32 dwJ;
	const f32 fRecip = 1.0f / 65536.0f;
	
	for (dwI = 0; dwI < 4; dwI++)
	{
		for (dwJ = 0; dwJ < 4; dwJ++)
		{
			s16 hi = *(s16 *)(g_pu8RamBase + ((address+(dwI<<3)+(dwJ<<1)     )^0x2));
			u16 lo = *(u16 *)(g_pu8RamBase + ((address+(dwI<<3)+(dwJ<<1) + 32)^0x2));

			mat.m[dwI][dwJ] = ((hi<<16) | lo ) * fRecip;
		}
	}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		DL_PF(
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n",
			mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
			mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
			mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
			mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
	}
#endif

	//mat.m[3][0] = mat.m[3][1] = mat.m[3][2] = 0;
	//mat.m[3][3] = 1;

	/*if (dwCommand & G_MTX_PROJECTION)
	{
		// So far only Extreme-G seems to Push/Pop projection matrices	
		mpRenderer->SetProjection(mat, bPush, nLoadCommand);
	}
	else*/
	//if (bPush)
	{
		mpRenderer->SetWorldView(mat, bPush, nLoadCommand);

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		mpRenderer->PrintActive();
#endif
	}
	/*else
	{

	}*/
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_PopMtx( MicroCodeCommand command )
{
	u8 nCommand = (u8)(command.cmd1 & 0xFF);

	DL_PF("    Command: 0x%02x (%s)",
		nCommand,  (nCommand & G_MTX_PROJECTION) ? "Projection" : "ModelView");

	// Do any of the other bits do anything?
	// So far only Extreme-G seems to Push/Pop projection matrices

	if (nCommand & G_MTX_PROJECTION)
	{
		mpRenderer->PopProjection();
	}
	else
	{
		mpRenderer->PopWorldView();
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

void DLParser_GBI2_Mtx( MicroCodeCommand command )
{	
	tMatrix mat;
	u32 dwI;
	u32 dwJ;
	const f32 fRecip = 1.0f / 65536.0f;
	u32 address = RDPSegAddr(command.cmd1);

	// THESE ARE SWAPPED OVER FROM NORMAL MTX
	// Not sure if this is right!!!!
	u32 dwCommand = (command.cmd0)&0xFF;
	u32 dwLength  = (command.cmd0 >> 8)&0xFFFF;

	use(dwLength);

	DL_PF("    Command: %s %s %s Length %d Address 0x%08x",
		(dwCommand & G_ZMTX_PROJECTION) ? "Projection" : "ModelView",
		(dwCommand & G_ZMTX_LOAD) ? "Load" : "Mul",	
		(dwCommand & G_ZMTX_NOPUSH) ? "NoPush" : "Push",
		dwLength, address);

	if (address + 64 > MAX_RAM_ADDRESS)
	{
		DBGConsole_Msg(0, "ZeldaMtx: Address invalid (0x%08x)", address);
		return;
	}

	// Load matrix from address	
	for (dwI = 0; dwI < 4; dwI++)
	{
		for (dwJ = 0; dwJ < 4; dwJ++) 
		{
			s16 hi = *(s16 *)(g_pu8RamBase + ((address+(dwI<<3)+(dwJ<<1)     )^0x2));
			u16 lo = *(u16 *)(g_pu8RamBase + ((address+(dwI<<3)+(dwJ<<1) + 32)^0x2));

			mat.m[dwI][dwJ] = ((hi<<16) | lo ) * fRecip;
		}
	}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		DL_PF(
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\r\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\r\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\r\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\r\n",
			mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
			mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
			mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
			mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
	}
#endif

	PVRRenderer::EMatrixLoadStyle nLoadCommand = (dwCommand & G_ZMTX_LOAD) ? PVRRenderer::MATRIX_LOAD : PVRRenderer::MATRIX_MUL;
	BOOL bPush = dwCommand & G_ZMTX_NOPUSH ? FALSE : TRUE;


	if (dwCommand & G_ZMTX_PROJECTION)
	{
		// So far only Extreme-G seems to Push/Pop projection matrices	
		mpRenderer->SetProjection(mat, bPush, nLoadCommand);
	}
	else
	{
		mpRenderer->SetWorldView(mat, bPush, nLoadCommand);
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_PopMtx( MicroCodeCommand command )
{
	u8 nCommand = (u8)(command.cmd0 & 0xFF);

	use(nCommand);

	DL_PF("        Command: 0x%02x (%s)", nCommand, (nCommand & G_ZMTX_PROJECTION) ? "Projection" : "ModelView");

/*	if (nCommand & G_ZMTX_PROJECTION)
	{
		mpRenderer->PopProjection();
	}
	else*/
	{
		mpRenderer->PopWorldView();
	}
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI0_Vtx_DKR( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.cmd1);
	u32 dwV0 =  0;
	u32 dwN  = ((command.cmd0 & 0xFFF) - 0x08) / 0x12;

	DL_PF("    Address 0x%08x, v0: %d, Num: %d", address, dwV0, dwN);

	if (dwV0 >= 32)
		dwV0 = 31;
	
	if ((dwV0 + dwN) > 32)
	{
		DL_PF("        Warning, attempting to load into invalid vertex positions");
		DBGConsole_Msg(0, "Warning, attempting to load into invalid vertex positions");
		dwN = 32 - dwV0;
	}

	// Check that address is valid...
	if ((address + (dwN*16)) > MAX_RAM_ADDRESS)
	{
		DBGConsole_Msg(0, "SetNewVertexInfo: Address out of range (0x%08x)", address);
	}
	else
	{
		mpRenderer->SetNewVertexInfoDKR(address, dwV0, dwN);

		g_dwNumVertices += dwN;

		DLParser_DumpVtxInfoDKR(address, dwV0, dwN);
	}
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI0_Vtx( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.cmd1);

	u32 len = (command.cmd0)&0xFFFF;
	u32 v0 =  (command.cmd0>>16)&0x0F;
	u32 n  = ((command.cmd0>>20)&0x0F)+1;

	use(len);

	DL_PF("    Address 0x%08x, v0: %d, Num: %d, Length: 0x%04x", address, v0, n, len);

	if ( (v0 + n) > 32 )
	{
		DL_PF("        Warning, attempting to load into invalid vertex positions");
		DBGConsole_Msg(0, "Warning, attempting to load into invalid vertex positions");
		return;
	}

	// Check that address is valid...
	if ( (address + (n*16)) > MAX_RAM_ADDRESS )
	{
		DBGConsole_Msg( 0, "SetNewVertexInfo: Address out of range (0x%08x)", address );
	}
	else
	{
		mpRenderer->SetNewVertexInfo( address, v0, n );

		g_dwNumVertices += n;

		DLParser_DumpVtxInfo( address, v0, n );
	}
}


//*****************************************************************************
// The previous way of calculating was based on the assumption that
// there was no "n" field. I didn't realise that the n/length fields shared the
// lower 16 bits (in a 7:9 split).
// u32 dwLength = (command.cmd0)&0xFFFF;
// u32 dwN      = (dwLength + 1) / 0x210;					// 528
// u32 dwV0     = ((command.cmd0>>16)&0xFF)/VertexStride;	// /5
//*****************************************************************************
void DLParser_GBI0_Vtx_WRUS( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.cmd1);
	
	u32 v0  = ((command.cmd0 >>16 ) & 0xff) / 5;
	u32 n   =  (command.cmd0 >>9  ) & 0x7f;
	u32 len =  (command.cmd0      ) & 0x1ff;

	use(len);

	DL_PF( "    Address 0x%08x, v0: %d, Num: %d, Length: 0x%04x", address, v0, n, len );

	if ( (v0 + n) > 32 )
	{
		DL_PF("    *Warning, attempting to load into invalid vertex positions");
		DBGConsole_Msg(0, "Warning, attempting to load into invalid vertex positions");
		return;
	}

	mpRenderer->SetNewVertexInfo( address, v0, n );

	g_dwNumVertices += n;

	DLParser_DumpVtxInfo( address, v0, n );
}

//*****************************************************************************
// The previous way of calculating was based on the assumption that
// there was no "n" field. I didn't realise that the n/length fields shared the
// lower 16 bits (in a 6:10 split).
// u32 dwLength = (command.cmd0)&0xFFFF;
// u32 dwN      = (dwLength + 1) / 0x210;					// 528
// u32 dwV0     = ((command.cmd0>>16)&0xFF)/VertexStride;	// /5
//*****************************************************************************
void DLParser_GBI1_Vtx( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.cmd1);

	//u32 dwLength = (command.cmd0)&0xFFFF;
	//u32 dwN      = (dwLength + 1) / 0x410;
	//u32 dwV0     = ((command.cmd0>>16)&0x3f)/2;

	u32 v0  = (command.cmd0 >>17 ) & 0x7f;		// ==((x>>16)&0xff)/2
	u32 n   = (command.cmd0 >>10 ) & 0x3f;
	u32 len = (command.cmd0      ) & 0x3ff;

	use(len);

	DL_PF("    Address 0x%08x, v0: %d, Num: %d, Length: 0x%04x", address, v0, n, len);

	if ( address > MAX_RAM_ADDRESS )
	{
		DL_PF("     Address out of range - ignoring load");
		return;
	}

	if ( (v0 + n) > 64 )
	{
		DL_PF("        Warning, attempting to load into invalid vertex positions");
		DBGConsole_Msg( 0, "        Warning, attempting to load into invalid vertex positions" );
		return;
	}

	mpRenderer->SetNewVertexInfo( address, v0, n );

	g_dwNumVertices += n;

	DLParser_DumpVtxInfo( address, v0, n );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_Vtx( MicroCodeCommand command )
{
	u32 address = RDPSegAddr(command.cmd1);
	u32 vend   = ((command.cmd0   )&0xFFF)/2;
	u32 n      = ((command.cmd0>>12)&0xFFF);

	u32 v0		= vend - n;

	DL_PF( "    Address 0x%08x, vEnd: %d, v0: %d, Num: %d", address, vend, v0, n );
	
	if ( vend > 64 )
	{
		DL_PF( "    *Warning, attempting to load into invalid vertex positions" );
		DBGConsole_Msg( 0, "Warning, attempting to load into invalid vertex positions: %d -> %d", v0, v0+n );
		return;
	}

	// Check that address is valid...
	if ( (address + (n*16) ) > MAX_RAM_ADDRESS )
	{
		DBGConsole_Msg( 0, "SetNewVertexInfo: Address out of range (0x%08x)", address );
	}
	else
	{
		mpRenderer->SetNewVertexInfo( address, v0, n );

		g_dwNumVertices += n;

		DLParser_DumpVtxInfo( address, v0, n );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_DumpVtxInfo(u32 address, u32 dwV0, u32 dwN)
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		s8 *pcSrc = (s8 *)(g_pu8RamBase + address);
		s16 *psSrc = (s16 *)(g_pu8RamBase + address);

		for ( u32 dwV = dwV0; dwV < dwV0 + dwN; dwV++ )
		{
			f32 x = f32(psSrc[0^0x1]);
			f32 y = f32(psSrc[1^0x1]);
			f32 z = f32(psSrc[2^0x1]);

			u16 wFlags = u16(mpRenderer->GetVtxFlags( dwV )); //(u16)psSrc[3^0x1];

			u8 a = pcSrc[12^0x3];
			u8 b = pcSrc[13^0x3];
			u8 c = pcSrc[14^0x3];
			u8 d = pcSrc[15^0x3];
			
			s16 nTU = psSrc[4^0x1];
			s16 nTV = psSrc[5^0x1];

			f32 tu = f32(nTU) / 32.0f;
			f32 tv = f32(nTV) / 32.0f;

			const v4 & t = mpRenderer->GetTransformedVtxPos( dwV );

			psSrc += 8;			// Increase by 16 bytes
			pcSrc += 16;

			DL_PF(" #%02d Flags: 0x%04x Pos: {% 6f,% 6f,% 6f} Tex: {%+7.2f,%+7.2f}, Extra: %02x %02x %02x %02x (transf: {% 6f,% 6f,% 6f})",
				dwV, wFlags, x, y, z, tu, tv, a, b, c, d, t.x, t.y, t.z );
		}
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
// DKR verts are extra 4 bytes
void DLParser_DumpVtxInfoDKR(u32 address, u32 dwV0, u32 dwN)
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		u32 dwV;
		s32 i;

		s16 * psSrc = (s16 *)(g_pu8RamBase + address);

		i = 0;
		for (dwV = dwV0; dwV < dwV0 + dwN; dwV++)
		{
			f32 x = f32(psSrc[(i + 0) ^ 1]);
			f32 y = f32(psSrc[(i + 1) ^ 1]);
			f32 z = f32(psSrc[(i + 2) ^ 1]);

			//u16 wFlags = mpRenderer->GetVtxFlags( dwV ); //(u16)psSrc[3^0x1];

			u16 wA = psSrc[(i + 3) ^ 1];
			u16 wB = psSrc[(i + 4) ^ 1];

			u8 a = u8(wA>>8);
			u8 b = u8(wA);
			u8 c = u8(wB>>8);
			u8 d = u8(wB);

			const v4 & t = mpRenderer->GetTransformedVtxPos( dwV );


			DL_PF(" #%02d Pos: {% 6f,% 6f,% 6f} Extra: %02x %02x %02x %02x (transf: {% 6f,% 6f,% 6f})",
				dwV, x, y, z, a, b, c, d, t.x, t.y, t.z );

			i+=5;
		}


		u16 * pwSrc = (u16 *)(g_pu8RamBase + address);
		i = 0;
		for (dwV = dwV0; dwV < dwV0 + dwN; dwV++)
		{
			DL_PF(" #%02d %04x %04x %04x %04x %04x",
				dwV, pwSrc[(i + 0) ^ 1],
				pwSrc[(i + 1) ^ 1],
				pwSrc[(i + 2) ^ 1],
				pwSrc[(i + 3) ^ 1],
				pwSrc[(i + 4) ^ 1]);
			
			i += 5;
		}

	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_DmaTri( MicroCodeCommand command )
{
	bool bTrisAdded = false;
	u32 address = RDPSegAddr(command.cmd1);
	u32 dwNum = ((command.cmd0 & 0xFF) / 0x10);
	u32 i;
	u32 * pData = &g_pu32RamBase[address/4];

	for (i = 0; i < dwNum; i++)
	{
		DL_PF("    0x%08x: %08x %08x %08x %08x", address + i*16, pData[0], pData[1], pData[2], pData[3]);

		u32 dwInfo = pData[ i ];

		u32 dwV0 = (dwInfo >> 16) & 0x1F;
		u32 dwV1 = (dwInfo >>  8) & 0x1F;
		u32 dwV2 = (dwInfo      ) & 0x1F;

		bTrisAdded |= mpRenderer->AddTri(dwV0, dwV1, dwV2);
	}

	if (bTrisAdded)	
	{
		if (g_bTextureEnable)
		{
			RDP_EnableTexturing(gTextureTile);
		}
		mpRenderer->FlushTris();
	}
}

//*****************************************************************************
//
//*****************************************************************************
template< int VertexStride > 
void DLParser_GBI1_Tri1_T( MicroCodeCommand command )
{
	// While the next command pair is Tri1, add vertices
	u32 pc = g_dwPCStack[g_dwPCStack.size() - 1].addr;
	u32 * pCmdBase = (u32 *)( g_pu8RamBase + pc );
	
	bool bTrisAdded = false;

	while (command.cmd == G_GBI1_TRI1)
	{
		//u32 dwFlag = (command.cmd1>>24)&0xFF;
		// Vertex indices are multiplied by 10 for Mario64, by 2 for MarioKart
		u32 dwV0 = ((command.cmd1>>16)&0xFF) / VertexStride;
		u32 dwV1 = ((command.cmd1>>8 )&0xFF) / VertexStride;
		u32 dwV2 = ((command.cmd1    )&0xFF) / VertexStride;

		bTrisAdded |= mpRenderer->AddTri(dwV0, dwV1, dwV2);

		command.cmd0= *pCmdBase++;
		command.cmd1= *pCmdBase++;
		pc += 8;

		if ( command.cmd == G_GBI1_TRI1 )
		{
			DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.cmd0, command.cmd1, gInstructionName[ command.cmd ]);
		}
	}

	g_dwPCStack[g_dwPCStack.size() - 1].addr = pc-8;

	if (bTrisAdded)	
	{
		if (g_bTextureEnable)
		{
			RDP_EnableTexturing(gTextureTile);
		}
		mpRenderer->FlushTris();
	}
}


//*****************************************************************************
/* Mariokart etc*/
//*****************************************************************************
template < int VertexStride >
void DLParser_GBI1_Tri2_T( MicroCodeCommand command )
{
	// While the next command pair is Tri2, add vertices
	u32 pc = g_dwPCStack[g_dwPCStack.size() - 1].addr;
	u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

	bool bTrisAdded = false;

	while (command.cmd == G_GBI1_TRI2)
	{
		// Vertex indices are multiplied by 10 for GBI0, by 2 for GBI1
		u32 dwV0 = ((command.cmd1>>16)&0xFF) / VertexStride;
		u32 dwV1 = ((command.cmd1>>8 )&0xFF) / VertexStride;
		u32 dwV2 = ((command.cmd1    )&0xFF) / VertexStride;

		u32 dwV3 = ((command.cmd0>>16)&0xFF) / VertexStride;
		u32 dwV4 = ((command.cmd0>>8 )&0xFF) / VertexStride;
		u32 dwV5 = ((command.cmd0    )&0xFF) / VertexStride;

		bTrisAdded |= mpRenderer->AddTri(dwV0, dwV1, dwV2);
		bTrisAdded |= mpRenderer->AddTri(dwV3, dwV4, dwV5);
		
		command.cmd0= *pCmdBase++;
		command.cmd1= *pCmdBase++;
		pc += 8;

		if ( command.cmd == G_GBI1_TRI2 )
		{
			DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.cmd0, command.cmd1, gInstructionName[ command.cmd ]);
		}
	}
	g_dwPCStack[g_dwPCStack.size() - 1].addr = pc-8;

	if (bTrisAdded)	
	{
		if (g_bTextureEnable)
		{
			RDP_EnableTexturing(gTextureTile);
		}

		mpRenderer->FlushTris();
	}
}

//*****************************************************************************
//
//*****************************************************************************
template< int VertexStride > 
void DLParser_GBI1_Line3D_T( MicroCodeCommand command )
{
	// While the next command pair is Tri1, add vertices
	u32 pc = g_dwPCStack[g_dwPCStack.size() - 1].addr;
	u32 * pCmdBase = (u32 *)( g_pu8RamBase + pc );

	bool bTrisAdded = false;

	while ( command.cmd == G_GBI1_LINE3D )
	{
		u32 dwV3   = ((command.cmd1>>24)&0xFF) / VertexStride;		
		u32 dwV0   = ((command.cmd1>>16)&0xFF) / VertexStride;
		u32 dwV1   = ((command.cmd1>>8 )&0xFF) / VertexStride;
		u32 dwV2   = ((command.cmd1    )&0xFF) / VertexStride;

		bTrisAdded |= mpRenderer->AddTri(dwV0, dwV1, dwV2);
		bTrisAdded |= mpRenderer->AddTri(dwV2, dwV3, dwV0);

		command.cmd0 = *pCmdBase++;
		command.cmd1 = *pCmdBase++;
		pc += 8;

		if ( command.cmd == G_GBI1_LINE3D )
		{
			DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.cmd0, command.cmd1, gInstructionName[ command.cmd ]);
		}
	} 

	g_dwPCStack[g_dwPCStack.size() - 1].addr = pc-8;

	if (bTrisAdded)	
	{
		if (g_bTextureEnable)
		{
			RDP_EnableTexturing(gTextureTile);
		}

		mpRenderer->FlushTris();
	}	
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI0_Tri2( MicroCodeCommand command )
{
	// While the next command pair is Tri2, add vertices
	u32 pc = g_dwPCStack[g_dwPCStack.size() - 1].addr;
	u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

	bool bTrisAdded = false;

	while (command.cmd == G_GBI1_TRI2)
	{
		// Vertex indices are exact
		u32 dwB = (command.cmd1    ) & 0xF;
		u32 dwA = (command.cmd1>> 4) & 0xF;
		u32 dwE = (command.cmd1>> 8) & 0xF;
		u32 dwD = (command.cmd1>>12) & 0xF;
		u32 dwH = (command.cmd1>>16) & 0xF;
		u32 dwG = (command.cmd1>>20) & 0xF;
		u32 dwK = (command.cmd1>>24) & 0xF;
		u32 dwJ = (command.cmd1>>28) & 0xF;

		u32 dwC = (command.cmd0    ) & 0xF;
		u32 dwF = (command.cmd0>> 4) & 0xF;
		u32 dwI = (command.cmd0>> 8) & 0xF;
		u32 dwL = (command.cmd0>>12) & 0xF;

		//u32 dwFlag = (command.cmd0>>16)&0xFF;

		// Don't check the first two tris for degenerates
		bTrisAdded |= mpRenderer->AddTri(dwA, dwC, dwB);
		bTrisAdded |= mpRenderer->AddTri(dwD, dwF, dwE);
		if (dwG != dwI && dwI != dwH && dwH != dwG)
		{
			bTrisAdded |= mpRenderer->AddTri(dwG, dwI, dwH);
		}

		if (dwJ != dwL && dwL != dwK && dwK != dwJ)
		{
			bTrisAdded |= mpRenderer->AddTri(dwJ, dwL, dwK);
		}
		
		command.cmd0= *pCmdBase++;
		command.cmd1= *pCmdBase++;
		pc += 8;

		if ( command.cmd == G_GBI1_TRI2 )
		{
			DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.cmd0, command.cmd1, gInstructionName[ command.cmd ]);
		}
	} 


	g_dwPCStack[g_dwPCStack.size() - 1].addr = pc-8;


	if (bTrisAdded)	
	{
		if (g_bTextureEnable)
		{
			RDP_EnableTexturing(gTextureTile);
		}

		mpRenderer->FlushTris();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_Tri1( MicroCodeCommand command )
{
	// While the next command pair is Tri1, add vertices
	u32 pc = g_dwPCStack[g_dwPCStack.size() - 1].addr;
	u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

	bool bTrisAdded = false;
	
	while ( command.cmd == G_GBI2_TRI1 )
	{
		//u32 dwFlag = (command.cmd1>>24)&0xFF;
		u32 dwV0 = ((command.cmd0    )&0xFF)/2;
		u32 dwV1 = ((command.cmd0>>8 )&0xFF)/2;
		u32 dwV2 = ((command.cmd0>>16)&0xFF)/2;

		bTrisAdded |= mpRenderer->AddTri(dwV0, dwV1, dwV2);

		command.cmd0 = *pCmdBase++;
		command.cmd1 = *pCmdBase++;
		pc += 8;

		if ( command.cmd == G_GBI2_TRI1 )
		{
			DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.cmd0, command.cmd1, gInstructionName[ command.cmd ]);
		}
	}
	g_dwPCStack[g_dwPCStack.size() - 1].addr = pc-8;

	if (bTrisAdded)	
	{
		if (g_bTextureEnable)
		{
			RDP_EnableTexturing(gTextureTile);
		}
		mpRenderer->FlushTris();
	}
}

//*****************************************************************************
// While the next command pair is Tri2, add vertices
//*****************************************************************************
void DLParser_GBI2_Tri2( MicroCodeCommand command )
{
	u32 pc = g_dwPCStack[g_dwPCStack.size() - 1].addr;
	u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

	bool bTrisAdded = false;

	while ( command.cmd == G_GBI2_TRI2 )
	{
		u32 dwV2 = ((command.cmd1>>16)&0xFF)/2;
		u32 dwV1 = ((command.cmd1>>8 )&0xFF)/2;
		u32 dwV0 = ((command.cmd1    )&0xFF)/2;

		u32 dwV5 = ((command.cmd0>>16)&0xFF)/2;
		u32 dwV4 = ((command.cmd0>>8 )&0xFF)/2;
		u32 dwV3 = ((command.cmd0    )&0xFF)/2;

		bTrisAdded |= mpRenderer->AddTri(dwV0, dwV1, dwV2);
		bTrisAdded |= mpRenderer->AddTri(dwV3, dwV4, dwV5);
		
		command.cmd0 = *pCmdBase++;
		command.cmd1 = *pCmdBase++;
		pc += 8;

		if ( command.cmd == G_GBI2_TRI2 )
		{
			DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.cmd0, command.cmd1, gInstructionName[ command.cmd ]);
		}
	}
	g_dwPCStack[g_dwPCStack.size() - 1].addr = pc-8;

	if (bTrisAdded)	
	{
		if (g_bTextureEnable)
		{
			RDP_EnableTexturing(gTextureTile);
		}

		mpRenderer->FlushTris();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_Line3D( MicroCodeCommand command )
{
	// While the next command pair is Tri2, add vertices
	u32 pc = g_dwPCStack[g_dwPCStack.size() - 1].addr;
	u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

	bool bTrisAdded = false;

	while ( command.cmd == G_GBI2_LINE3D )
	{
		// Vertex indices are multiplied by 10 for Mario64, by 2 for MarioKart
		u32 dwV2 = ((command.cmd1>>16)&0xFF)/2;
		u32 dwV1 = ((command.cmd1>>8 )&0xFF)/2;
		u32 dwV0 = ((command.cmd1    )&0xFF)/2;

		u32 dwV5 = ((command.cmd0>>16)&0xFF)/2;
		u32 dwV4 = ((command.cmd0>>8 )&0xFF)/2;
		u32 dwV3 = ((command.cmd0    )&0xFF)/2;

		bTrisAdded |= mpRenderer->AddTri(dwV0, dwV1, dwV2);
		bTrisAdded |= mpRenderer->AddTri(dwV3, dwV4, dwV5);
	
		command.cmd0 = *pCmdBase++;
		command.cmd1 = *pCmdBase++;
		pc += 8;

		if ( command.cmd == G_GBI2_LINE3D )
		{
			DL_PF("0x%08x: %08x %08x %-10s", pc-8, command.cmd0, command.cmd1, gInstructionName[ command.cmd ]);
		}
	}

	g_dwPCStack[g_dwPCStack.size() - 1].addr = pc-8;

	if (bTrisAdded)	
	{
		if (g_bTextureEnable)
		{
			RDP_EnableTexturing(gTextureTile);
		}
		mpRenderer->FlushTris();
	}
}



//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetScissor( MicroCodeCommand command )
{
	// The coords are all in 8:2 fixed point
	u32 x0   = (command.cmd0>>12)&0xFFF;
	u32 y0   = (command.cmd0>>0 )&0xFFF;
	u32 mode = (command.cmd1>>24)&0x03;
	u32 x1   = (command.cmd1>>12)&0xFFF;
	u32 y1   = (command.cmd1>>0 )&0xFFF;

	use(mode);

	DL_PF("    x0=%d y0=%d x1=%d y1=%d mode=%d", x0/4, y0/4, x1/4, y1/4, mode);

	// Set the cliprect now...
	if ( x0 < x1 && y0 < y1 )
	{
		mpRenderer->SetScissor( x0/4, y0/4, x1/4, y1/4 );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetTile( MicroCodeCommand command )
{
	RDP_Tile tile;
	tile.cmd0 = command.cmd0;
	tile.cmd1 = command.cmd1;
	
	RDP_SetTile( tile );
	gRDPStateManager.SetTile( tile.tile_idx, tile );

	DL_PF("    Tile:%d  Fmt: %s/%s Line:%d TMem:0x%04x Palette:%d", tile.tile_idx, gFormatNames[tile.format], gSizeNames[tile.size], tile.line, tile.tmem, tile.palette);
	DL_PF( "         S: Clamp: %s Mirror:%s Mask:0x%x Shift:0x%x", gOnOffNames[tile.clamp_s],gOnOffNames[tile.mirror_s], tile.mask_s, tile.shift_s );
	DL_PF( "         T: Clamp: %s Mirror:%s Mask:0x%x Shift:0x%x", gOnOffNames[tile.clamp_t],gOnOffNames[tile.mirror_t], tile.mask_t, tile.shift_t );

}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetTileSize( MicroCodeCommand command )
{
	RDP_TileSize tile;
	tile.cmd0 = command.cmd0;
	tile.cmd1 = command.cmd1;

	RDP_SetTileSize( tile );

	DL_PF("    Tile:%d (%d,%d) -> (%d,%d) [%d x %d]",
		tile.tile_idx, tile.left/4, tile.top/4,
		        tile.right/4, tile.bottom/4,
				((tile.right/4) - (tile.left/4)) + 1,
				((tile.bottom/4) - (tile.top/4)) + 1);

	static bool skip_next = false;
	if (skip_next)
	{
		skip_next = false;

		DL_PF("Tetrisphere hack! - skipping command");
		return;
	}

	// Wetrix hack
	if (tile.top > tile.bottom || tile.left > tile.right)
	{
		DL_PF("             Ignoring - negative size");
		return;
	}

	gRDPStateManager.SetTileSize( tile.tile_idx, tile );

	// Testrisphere hack - ignore second SetTileSize
	//SetTileSize: 0xf20001b0 0x004fc1c4 Tile:0 (0,108) -> (319,113) [320 x 6]
	//             (0,432) -> (1276, 452) [1277 x 21]
	//SetTileSize: 0xf2000000 0x004ff017 Tile:0 (0,0) -> (319,5) [320 x 6]
	//             (0,0) -> (1279, 23) [1280 x 24]

	u32 dwPC = g_dwPCStack[g_dwPCStack.size() - 1].addr;

	u32 dwNextCmd0			= *(u32 *)(g_pu8RamBase + dwPC+0);
	u32 dwNextCmd1			= *(u32 *)(g_pu8RamBase + dwPC+4);

	if ((dwNextCmd0>>24) == (u8)G_RDP_SETTILESIZE &&
		(dwNextCmd1>>24) == tile.tile_idx)
	{
		// Skip it!
		DAEDALUS_ASSERTMSG( "Applying TS Skip hack" );

		skip_next = true;
	}
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetTImg( MicroCodeCommand command )
{
	g_TI.Format		= (command.cmd0>>21)&0x7;
	g_TI.Size		= (command.cmd0>>19)&0x3;
	g_TI.Width		= (command.cmd0&0x0FFF) + 1;
	g_TI.Address	= RDPSegAddr(command.cmd1);

	DL_PF("    Image: 0x%08x Fmt: %s/%s Width: %d", g_TI.Address,
		gFormatNames[g_TI.Format], gSizeNames[g_TI.Size], g_TI.Width);
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_LoadBlock( MicroCodeCommand command )
{
	u32 dwULS		= ((command.cmd0>>12)&0x0FFF)/4;
	u32 dwULT		= ((command.cmd0    )&0x0FFF)/4;
	//u32 dwPad     =  (command.cmd1>>27)&0x1f;
	u32 tile_idx	=  (command.cmd1>>24)&0x07;
	u32 dwPixels	= ((command.cmd1>>12)&0x0FFF) + 1;		// Number of bytes-1?
	u32 dwDXT		=  (command.cmd1    )&0x0FFF;		// 1.11 fixed point

	use(dwPixels);

	u32		quadwords;
	bool	swapped;

	if (dwDXT == 0)
	{
		quadwords = 1;
		swapped = true;
	}
	else
	{
		quadwords = 2048 / dwDXT;						// #Quad Words
		swapped = false;
	}

	u32		dwBytes = quadwords * 8;
	u32		dwWidth = (dwBytes*2)>>g_TI.Size;
	u32		dwPixelOffset = (dwWidth * dwULT) + dwULS;
	u32		dwOffset = (dwPixelOffset<<g_TI.Size)/2;
	u32		dwSrcOffset = (g_TI.Address + dwOffset);

	DL_PF("    Tile:%d (%d,%d) %d pixels DXT:0x%04x = %d QWs => %d pixels/line", tile_idx, dwULS, dwULT, dwPixels, dwDXT, quadwords, dwWidth);
	DL_PF("    Offset: 0x%08x", dwSrcOffset);

	gRDPStateManager.LoadBlock( tile_idx, dwSrcOffset, ~0, swapped );

	RDP_TileSize tile;
	tile.cmd0 = command.cmd0;
	tile.cmd1 = command.cmd1;
	RDP_LoadBlock( tile );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_LoadTile( MicroCodeCommand command )
{
	u32 dwULS		= ((command.cmd0>>12)&0x0FFF)/4;
	u32 dwULT		= ((command.cmd0    )&0x0FFF)/4;
	u32 tile_idx	=  (command.cmd1>>24)&0x07;
	u32 dwLRS		= ((command.cmd1>>12)&0x0FFF)/4;
	u32 dwLRT		= ((command.cmd1    )&0x0FFF)/4;

	use(dwLRS);
	use(dwLRT);

	// Todo - check if this is odd, and round up?
	u32 dwPitch = ((g_TI.Width<<g_TI.Size)+1)/2;
	u32 dwOffset = ( dwPitch * dwULT ) + dwULS;
	u32 dwSrcOffset = (g_TI.Address + dwOffset);

	use(dwSrcOffset);
	
	DL_PF("    Tile:%d (%d,%d) -> (%d,%d) [%d x %d]",			tile_idx, dwULS, dwULT, dwLRS, dwLRT, (dwLRS - dwULS)+1, (dwLRT - dwULT)+1);
	DL_PF("    g_TI.Address + dwOffset: 0x%08x (LoadTile Pitch: %d)",		dwSrcOffset, dwPitch);

	gRDPStateManager.LoadTile( tile_idx, g_TI.Address, dwPitch );

	RDP_TileSize tile;
	tile.cmd0 = command.cmd0;
	tile.cmd1 = command.cmd1;
	RDP_LoadTile( tile );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_LoadTLut( MicroCodeCommand command )
{
	u32 dwOffset;
	u32 dwULS   = ((command.cmd0 >> 12) & 0xfff)/4;
	u32 dwULT   = ((command.cmd0      ) & 0xfff)/4;
	u32 tile_idx= ((command.cmd1 >> 24) & 0x07);
	u32 dwLRS   = ((command.cmd1 >> 12) & 0xfff)/4;
	u32 dwLRT   = ((command.cmd1      ) & 0xfff)/4;
	u32 dwCount = (dwLRS - dwULS);

	use(tile_idx);
	use(dwLRS);
	use(dwLRT);
	use(dwCount);

	// Format is always 16bpp - RGBA16 or IA16:
	// I've no idea why these two are added - seems to work for 007!
	dwOffset = (dwULS + dwULT)*2;

	//const RDP_Tile &	rdp_tile( gRDPStateManager.GetTile( tile_idx ) );
	//gPalAddresses[ rdp_tile.tmem ] = g_TI.Address + dwOffset;

	RDP_TileSize tile;
	tile.cmd0 = command.cmd0;
	tile.cmd1 = command.cmd1;

	RDP_LoadTLut( tile );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		char str[300] = "";
		char item[300];
		u16 * pBase = (u16 *)(g_pu8RamBase + (g_TI.Address + dwOffset));
		u32 i;


		DL_PF("    LoadTLut Tile:%d, (%d,%d) -> (%d,%d), Count %d",
			tile_idx, dwULS, dwULT, dwLRS, dwLRT, dwCount);
		// This is sometimes wrong (in 007) tlut fmt is set after 
		// tlut load, but before tile load
		DL_PF("    Fmt is %s", gTLUTTypeName[gRDPOtherMode.text_tlut]);

		for (i = 0; i < dwCount; i++)
		{
			u16 wEntry = pBase[i ^ 0x1];

			if (i % 8 == 0)
			{
				DL_PF(str);

				// Clear
				sprintf(str, "%03d: ", i);
			}

			sprintf(item, "0x%04x (0x%08x) ", wEntry, Convert555ToRGBA(wEntry));
			strcat(str, item);
		}
		DL_PF(str);
	}
#endif
}



//*****************************************************************************
//
//*****************************************************************************
inline f32 round( f32 x )
{
	return (f32)(s32)( x + 0.5f );
}

void DLParser_TexRect( MicroCodeCommand command )
{
	RDP_TexRect tex_rect;
	MicroCodeCommand command2;
	MicroCodeCommand command3;

	//
	// Fetch the next two instructions
	//
	DLParser_FetchNextCommand( &command2 );
	DLParser_FetchNextCommand( &command3 );

	tex_rect.cmd0 = command.cmd0;
	tex_rect.cmd1 = command.cmd1;
	tex_rect.cmd2 = command2.cmd1;
	tex_rect.cmd3 = command3.cmd1;

	tVector2 d( tex_rect.dsdx / 1024.0f, tex_rect.dtdy / 1024.0f );
	tVector2 xy0( tex_rect.x0 / 4.0f, tex_rect.y0 / 4.0f );
	tVector2 xy1( tex_rect.x1 / 4.0f, tex_rect.y1 / 4.0f );
	tVector2 uv0( tex_rect.s / 32.0f, tex_rect.t / 32.0f );
	tVector2 uv1;

	if ((g_dwOtherModeH & G_CYC_COPY) == G_CYC_COPY)
	{
		d.x /= 4.0f;	// In copy mode 4 pixels are copied at once.
	}

	const RDP_TileSize &	rdp_tilesize( gRDPStateManager.GetTileSize( tex_rect.tile_idx ) );
	f32		tile_left( rdp_tilesize.left / 4.0f );
	f32		tile_top( rdp_tilesize.top / 4.0f );

	//
	// When texrecting large backgrounds, fS0/fT0 will start with a u/v corresponding to the tile's top/left texel coord
	//
	if (tex_rect.s != 0)	uv0.x -= tile_left;
	if (tex_rect.t != 0)	uv0.y -= tile_top;

	//
	// We round these value here, so that we get nice texturemapping when tiling
	// backgrounds. This is pretty noticeable on MKrt and TSph
	//
	xy0.x = round( xy0.x );
	xy0.y = round( xy0.y );

	xy1.x = round( xy1.x );
	xy1.y = round( xy1.y );

	uv1.x = uv0.x + d.x * ( xy1.x - xy0.x );
	uv1.y = uv0.y + d.y * ( xy1.y - xy0.y );

#ifdef DEBUG_TEXRECT
	DL_PF("    Tile:%d Screen(%f,%f) -> (%f,%f)",				   tex_rect.tile_idx, xy0.x, xy0.y, xy1.x, xy1.y);
	DL_PF("           Tex:(%#5f,%#5f) -> (%#5f,%#5f) (DSDX:%#5f DTDY:%#5f)",          uv0.x, uv0.y, uv1.x, uv1.y, d.x, d.y);
	DL_PF("");
#endif

	RDP_EnableTexturing( tex_rect.tile_idx );

	mpRenderer->TexRect( xy0, xy1, uv0, uv1 );
}

//*****************************************************************************
//
//*****************************************************************************

void DLParser_TexRectFlip( MicroCodeCommand command )
{ 
	RDP_TexRect tex_rect;
	MicroCodeCommand command2;
	MicroCodeCommand command3;

	//
	// Fetch the next two instructions
	//
	DLParser_FetchNextCommand( &command2 );
	DLParser_FetchNextCommand( &command3 );

	tex_rect.cmd0 = command.cmd0;
	tex_rect.cmd1 = command.cmd1;
	tex_rect.cmd2 = command2.cmd1;
	tex_rect.cmd3 = command3.cmd1;

	tVector2 d( tex_rect.dsdx / 1024.0f, tex_rect.dtdy / 1024.0f );
	tVector2 xy0( tex_rect.x0 / 4.0f, tex_rect.y0 / 4.0f );
	tVector2 xy1( tex_rect.x1 / 4.0f, tex_rect.y1 / 4.0f );
	tVector2 uv0( tex_rect.s / 32.0f, tex_rect.t / 32.0f );
	tVector2 uv1;

	if ((g_dwOtherModeH & G_CYC_COPY) == G_CYC_COPY)
	{
		d.x /= 4.0f;	// In copy mode 4 pixels are copied at once.
	}

	const RDP_TileSize &	rdp_tilesize( gRDPStateManager.GetTileSize( tex_rect.tile_idx ) );
	f32		tile_left( rdp_tilesize.left / 4.0f );
	f32		tile_top( rdp_tilesize.top / 4.0f );

	//
	// When texrecting large backgrounds, fS0/fT0 will start with a u/v corresponding to the tile's top/left texel coord
	//
	if (tex_rect.s != 0)	uv0.x -= tile_left;
	if (tex_rect.t != 0)	uv0.y -= tile_top;
	
	//
	// We round these value here, so that we get nice texturemapping when tiling
	// backgrounds. This is pretty noticeable on MKrt and TSph
	//
	xy0.x = round( xy0.x );
	xy0.y = round( xy0.y );

	xy1.x = round( xy1.x );
	xy1.y = round( xy1.y );

	uv1.x = uv0.x + d.x * ( xy1.y - xy0.y );		// Flip - use y
	uv1.y = uv0.y + d.y * ( xy1.x - xy0.x );		// Flip - use x

#ifdef DEBUG_TEXRECT
	DL_PF("    Tile:%d Screen(%f,%f) -> (%f,%f)",				   tex_rect.tile_idx, xy0.x, xy0.y, xy1.x, xy1.y);
	DL_PF("           Tex:(%#5f,%#5f) -> (%#5f,%#5f) (DSDX:%#5f DTDY:%#5f)",          uv0.x, uv0.y, uv1.x, uv1.y, d.x, d.y);
	DL_PF("");
#endif
	
	RDP_EnableTexturing( tex_rect.tile_idx );
	
	mpRenderer->TexRectFlip( xy0, xy1, uv0, uv1 );
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_FillRect( MicroCodeCommand command )
{ 
	u32 x0   = (command.cmd1>>12)&0xFFF;
	u32 y0   = (command.cmd1>>0 )&0xFFF;
	u32 x1   = (command.cmd0>>12)&0xFFF;
	u32 y1   = (command.cmd0>>0 )&0xFFF;

	tVector2 xy0( x0 / 4.0f, y0 / 4.0f );
	tVector2 xy1( x1 / 4.0f, y1 / 4.0f );

	// Note, in some modes, the right/bottom lines aren't drawn

	DL_PF("    (%d,%d) (%d,%d)", x0, y0, x1, y1);

	// TODO - In 1/2cycle mode, skip bottom/right edges!?

	if (g_DI.Address == g_CI.Address)
	{
		// Clear the Z Buffer
		//s_pD3DX->SetClearDepth(depth from set fill color);
		/*KOS
		CGraphicsContext::Get()->Clear( false, true );*/

		DL_PF("    Clearing ZBuffer");
	}
	else
	{
		// Clear the screen if large rectangle?
		// This seems to mess up with the Zelda game select screen
		// For some reason it draws a large rect over the entire
		// display, right at the end of the dlist. It sets the primitive
		// colour just before, so maybe I'm missing something??
		{

			/*if (((x1+1)-x0) == g_dwViWidth &&
				((y1+1)-y0) == g_dwViHeight &&
				g_dwFillColor == 0xFF000000)
			{
				DL_PF("  Filling Rectangle (screen clear)");
				CGraphicsContext::Get()->Clear(true, false);

				//DBGConsole_Msg(0, "DL ColorBuffer = 0x%08x", g_CI.Address);
			}
			else*/
			{
				// TODO - Check colour image format to work out how this ahould be decoded!

				u32 color;
				
				if ( g_CI.Size == G_IM_SIZ_16b )
				{
					color = Convert555ToRGBA((u16)g_dwFillColor);
				}
				else
				{
					color = g_dwFillColor;
				}
				DL_PF("    Filling Rectangle");
				xy1 += tVector2( 1.0f, 1.0f );
				mpRenderer->FillRect( xy0, xy1, color );
			}
		}

	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Texture( MicroCodeCommand command )
{
	//u32 dwBowTie = (command.cmd0>>16)&0xFF;		// Don't care about this
	gTextureLevel = (command.cmd0>>11)&0x07;
	gTextureTile  = (command.cmd0>>8 )&0x07;

	bool enable =    ((command.cmd0    )&0xFF) != 0;			// Seems to use 0x01
	f32 scale_s = f32((command.cmd1>>16)&0xFFFF) / (65536.0f * 32.0f);
	f32 scale_t = f32((command.cmd1    )&0xFFFF) / (65536.0f * 32.0f);

	DL_PF("    Level: %d Tile: %d %s", gTextureLevel, gTextureTile, enable ? "enabled":"disabled");
	DL_PF("    ScaleS: %f, ScaleT: %f", scale_s*32.0f, scale_t*32.0f);

	mpRenderer->SetTextureEnable( enable );
	mpRenderer->SetTextureScale( scale_s, scale_t );
	g_bTextureEnable = enable;

}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_Texture( MicroCodeCommand command )
{
	//u32 dwBowTie = (command.cmd0>>16)&0xFF;		// Don't care about this
	gTextureLevel = (command.cmd0>>11)&0x07;
	gTextureTile  = (command.cmd0>>8 )&0x07;

	bool enable =    ((command.cmd0    )&0xFF) != 0;			// Seems to use 0x02
	f32 scale_s = f32((command.cmd1>>16)&0xFFFF) / (65536.0f * 32.0f);
	f32 scale_t = f32((command.cmd1    )&0xFFFF) / (65536.0f * 32.0f);

	DL_PF("    Level: %d Tile: %d %s", gTextureLevel, gTextureTile, enable ? "enabled":"disabled");
	DL_PF("    ScaleS: %f, ScaleT: %f", scale_s*32.0f, scale_t*32.0f);

	mpRenderer->SetTextureEnable( enable );
	mpRenderer->SetTextureScale( scale_s, scale_t );
	g_bTextureEnable = enable;
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetZImg( MicroCodeCommand command )
{
	DL_PF("    Image: 0x%08x", RDPSegAddr(command.cmd1));

	g_DI.Address = RDPSegAddr(command.cmd1);
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetCImg( MicroCodeCommand command )
{
	u32 format = (command.cmd0>>21)&0x7;
	u32 size   = (command.cmd0>>19)&0x3;
	u32 width  = (command.cmd0&0x0FFF) + 1;

	DL_PF("    Image: 0x%08x", RDPSegAddr(command.cmd1));
	DL_PF("    Fmt: %s Size: %s Width: %d", gFormatNames[ format ], gSizeNames[ size ], width);

	g_CI.Address = RDPSegAddr(command.cmd1);
	g_CI.Format = format;
	g_CI.Size = size;
	g_CI.Width = width;
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_SetOtherModeL( MicroCodeCommand command )
{
	u32 shift  = (command.cmd0>>8)&0xFF;
	u32 length = (command.cmd0   )&0xFF;
	u32 data   =  command.cmd1;

	u32 mask = ((1<<length)-1)<<shift;

	g_dwOtherModeL = (g_dwOtherModeL&(~mask)) | data;

	RDP_SetOtherMode( g_dwOtherModeH, g_dwOtherModeL );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_SetOtherModeH( MicroCodeCommand command )
{
	u32 shift  = (command.cmd0>>8)&0xFF;
	u32 length = (command.cmd0   )&0xFF;
	u32 data   =  command.cmd1;

	u32 mask = ((1<<length)-1)<<shift;

	g_dwOtherModeH = (g_dwOtherModeH&(~mask)) | data;

	RDP_SetOtherMode( g_dwOtherModeH, g_dwOtherModeL );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_SetOtherModeL( MicroCodeCommand command )
{
	u32 shift  = (command.cmd0>>8)&0xFF;
	u32 length = (command.cmd0   )&0xFF;
	u32 data   =  command.cmd1;

	// Mask is constructed slightly differently
	u32 mask = (u32)((s32)(0x80000000)>>length)>>shift;

	g_dwOtherModeL = (g_dwOtherModeL&(~mask)) | data;
	
	RDP_SetOtherMode( g_dwOtherModeH, g_dwOtherModeL );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI2_SetOtherModeH( MicroCodeCommand command )
{
	u32 shift  = (command.cmd0>>8)&0xFF;
	u32 length = (command.cmd0   )&0xFF;
	u32 data   =  command.cmd1;

	// Mask is constructed slightly differently
	u32 mask = (u32)((s32)(0x80000000)>>length)>>shift;

	g_dwOtherModeH = (g_dwOtherModeH&(~mask)) | data;

	RDP_SetOtherMode( g_dwOtherModeH, g_dwOtherModeL );
}

//*****************************************************************************
//
//*****************************************************************************
static void DLParser_InitGeometryMode()
{
	bool bCullFront		= (g_dwGeometryMode & G_CULL_FRONT) ? true : false;
	bool bCullBack		= (g_dwGeometryMode & G_CULL_BACK) ? true : false;
	
	bool bShade			= (g_dwGeometryMode & G_SHADE) ? true : false;
	bool bShadeSmooth	= (g_dwGeometryMode & G_SHADING_SMOOTH) ? true : false;
	
	bool bFog			= (g_dwGeometryMode & G_FOG) ? true : false;
	bool bTextureGen	= (g_dwGeometryMode & G_TEXTURE_GEN) ? true : false;

	bool bLighting      = (g_dwGeometryMode & G_LIGHTING) ? true : false;
	bool bZBuffer		= (g_dwGeometryMode & G_ZBUFFER) ? true : false;	

	mpRenderer->SetCullMode(bCullFront, bCullBack);
	
	mpRenderer->SetSmooth( bShade );
	mpRenderer->SetSmoothShade( bShadeSmooth );
	
	mpRenderer->SetFogEnable( bFog );
	mpRenderer->SetTextureGen(bTextureGen);

	mpRenderer->SetLighting( bLighting );
	mpRenderer->ZBufferEnable( bZBuffer );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_ClearGeometryMode( MicroCodeCommand command )
{
	u32 dwMask = (command.cmd1);
	
	g_dwGeometryMode &= ~dwMask;

	//g_bTextureEnable = (g_dwGeometryMode & G_TEXTURE_ENABLE);

	DLParser_InitGeometryMode();

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		DL_PF("    Mask=0x%08x", dwMask);
		if (dwMask & G_ZBUFFER)						DL_PF("  Disabling ZBuffer");
		if (dwMask & G_TEXTURE_ENABLE)				DL_PF("  Disabling Texture");
		if (dwMask & G_SHADE)						DL_PF("  Disabling Shade");
		if (dwMask & G_SHADING_SMOOTH)				DL_PF("  Disabling Smooth Shading");
		if (dwMask & G_CULL_FRONT)					DL_PF("  Disabling Front Culling");
		if (dwMask & G_CULL_BACK)					DL_PF("  Disabling Back Culling");
		if (dwMask & G_FOG)							DL_PF("  Disabling Fog");
		if (dwMask & G_LIGHTING)					DL_PF("  Disabling Lighting");
		if (dwMask & G_TEXTURE_GEN)					DL_PF("  Disabling Texture Gen");
		if (dwMask & G_TEXTURE_GEN_LINEAR)			DL_PF("  Disabling Texture Gen Linear");
		if (dwMask & G_LOD)							DL_PF("  Disabling LOD (no impl)");
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_SetGeometryMode(  MicroCodeCommand command  )
{
	u32 dwMask = command.cmd1;

	g_dwGeometryMode |= dwMask;

	//g_bTextureEnable = (g_dwGeometryMode & G_TEXTURE_ENABLE);

	DLParser_InitGeometryMode();

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		DL_PF("    Mask=0x%08x", dwMask);
		if (dwMask & G_ZBUFFER)						DL_PF("  Enabling ZBuffer");
		if (dwMask & G_TEXTURE_ENABLE)				DL_PF("  Enabling Texture");
		if (dwMask & G_SHADE)						DL_PF("  Enabling Shade");
		if (dwMask & G_SHADING_SMOOTH)				DL_PF("  Enabling Smooth Shading");
		if (dwMask & G_CULL_FRONT)					DL_PF("  Enabling Front Culling");
		if (dwMask & G_CULL_BACK)					DL_PF("  Enabling Back Culling");
		if (dwMask & G_FOG)							DL_PF("  Enabling Fog");
		if (dwMask & G_LIGHTING)					DL_PF("  Enabling Lighting");
		if (dwMask & G_TEXTURE_GEN)					DL_PF("  Enabling Texture Gen");
		if (dwMask & G_TEXTURE_GEN_LINEAR)			DL_PF("  Enabling Texture Gen Linear");
		if (dwMask & G_LOD)							DL_PF("  Enabling LOD (no impl)");
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
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

#define G_ZELDA_ZBUFFER			G_ZBUFFER		// Guess
#define G_ZELDA_CULL_BACK		/*G_CULL_FRONT //*/ 0x00000200
#define G_ZELDA_CULL_FRONT		/*G_CULL_BACK  //*/ 0x00000400
#define G_ZELDA_FOG				G_FOG
#define G_ZELDA_LIGHTING		G_LIGHTING
#define G_ZELDA_TEXTURE_GEN		G_TEXTURE_GEN
#define G_ZELDA_SHADING_FLAT	G_TEXTURE_GEN_LINEAR
//#define G_ZELDA_SHADE			0x00080000
#define G_CLIPPING				0x00800000


// Seems to be AND (command.cmd0&0xFFFFFF) OR (command.cmd1&0xFFFFFF)
void DLParser_GBI2_GeometryMode( MicroCodeCommand command )
{
	u32 dwAnd = (command.cmd0) & 0x00FFFFFF;
	u32 dwOr  = (command.cmd1) & 0x00FFFFFF;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		DL_PF("    0x%08x 0x%08x =(x & 0x%08x) | 0x%08x", command.cmd0, command.cmd1, dwAnd, dwOr);

		if ((~dwAnd) & G_ZELDA_ZBUFFER)					DL_PF("  Disabling ZBuffer");
	//	if ((~dwAnd) & G_ZELDA_TEXTURE_ENABLE)			DL_PF("  Disabling Texture");
	//	if ((~dwAnd) & G_ZELDA_SHADE)					DL_PF("  Disabling Shade");
		if ((~dwAnd) & G_ZELDA_SHADING_FLAT)			DL_PF("  Disabling Flat Shading");
		if ((~dwAnd) & G_ZELDA_CULL_FRONT)				DL_PF("  Disabling Front Culling");
		if ((~dwAnd) & G_ZELDA_CULL_BACK)				DL_PF("  Disabling Back Culling");
		if ((~dwAnd) & G_ZELDA_FOG)						DL_PF("  Disabling Fog");
		if ((~dwAnd) & G_ZELDA_LIGHTING)				DL_PF("  Disabling Lighting");
		if ((~dwAnd) & G_ZELDA_TEXTURE_GEN)				DL_PF("  Disabling Texture Gen");
	//	if ((~dwAnd) & G_ZELDA_TEXTURE_GEN_LINEAR)		DL_PF("  Disabling Texture Gen Linear");
	//	if ((~dwAnd) & G_ZELDA_LOD)						DL_PF("  Disabling LOD (no impl)");

		if (dwOr & G_ZELDA_ZBUFFER)						DL_PF("  Enabling ZBuffer");
	//	if (dwOr & G_ZELDA_TEXTURE_ENABLE)				DL_PF("  Enabling Texture");
	//	if (dwOr & G_ZELDA_SHADE)						DL_PF("  Enabling Shade");
		if (dwOr & G_ZELDA_SHADING_FLAT)				DL_PF("  Enabling Flat Shading");
		if (dwOr & G_ZELDA_CULL_FRONT)					DL_PF("  Enabling Front Culling");
		if (dwOr & G_ZELDA_CULL_BACK)					DL_PF("  Enabling Back Culling");
		if (dwOr & G_ZELDA_FOG)							DL_PF("  Enabling Fog");
		if (dwOr & G_ZELDA_LIGHTING)					DL_PF("  Enabling Lighting");
		if (dwOr & G_ZELDA_TEXTURE_GEN)					DL_PF("  Enabling Texture Gen");
	//	if (dwOr & G_ZELDA_TEXTURE_GEN_LINEAR)			DL_PF("  Enabling Texture Gen Linear");
	//	if (dwOr & G_ZELDA_LOD)							DL_PF("  Enabling LOD (no impl)");

	}
#endif

	g_dwGeometryMode &= dwAnd;
	g_dwGeometryMode |= dwOr;


	bool bCullFront		= (g_dwGeometryMode & G_ZELDA_CULL_FRONT) ? true : false;
	bool bCullBack		= (g_dwGeometryMode & G_ZELDA_CULL_BACK) ? true : false;
	
	//bool bShade			= (g_dwGeometryMode & G_ZELDA_SHADE) ? true : false;
	bool bFlatShade		= (g_dwGeometryMode & G_ZELDA_SHADING_FLAT) ? true : false;
	
	bool bFog			= (g_dwGeometryMode & G_ZELDA_FOG) ? true : false;
	bool bTextureGen	= (g_dwGeometryMode & G_ZELDA_TEXTURE_GEN) ? true : false;

	bool bLighting      = (g_dwGeometryMode & G_ZELDA_LIGHTING) ? true : false;
	bool bZBuffer		= (g_dwGeometryMode & G_ZELDA_ZBUFFER)	? true : false;	

	mpRenderer->SetCullMode(bCullFront, bCullBack);
		
	mpRenderer->SetSmooth( !bFlatShade );
	mpRenderer->SetSmoothShade( true );		// Always do this - not sure which bit to use
	
	mpRenderer->SetFogEnable( bFog );
	mpRenderer->SetTextureGen(bTextureGen);

	mpRenderer->SetLighting( bLighting );
	mpRenderer->ZBufferEnable( bZBuffer );

	//DLParser_InitGeometryMode();
}



//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetCombine( MicroCodeCommand command )
{
	u32 dwMux0 = command.cmd0&0x00FFFFFF;
	u32 dwMux1 = command.cmd1;

	u64 mux = (((u64)dwMux0) << 32) | (u64)dwMux1;

	RDP_SetMux( mux );
	
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		DLParser_DumpMux( (((u64)dwMux0) << 32) | (u64)dwMux1 );
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetFillColor( MicroCodeCommand command )
{
	g_dwFillColor = command.cmd1;

	DL_PF( "    Color5551=0x%04x = 0x%08x", (u16)command.cmd1, Convert555ToRGBA((u16)command.cmd1) );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetFogColor( MicroCodeCommand command )
{
	u32 r	= N64COL_GETR(command.cmd1);
	u32 g	= N64COL_GETG(command.cmd1);
	u32 b	= N64COL_GETB(command.cmd1);
	u32 a	= N64COL_GETA(command.cmd1);

	DL_PF("    RGBA: %d %d %d %d", r, g, b, a);

	//u32 dwFogColor = command.cmd1;

	mpRenderer->SetFogColor( RGBA_MAKE(r, g, b, a) );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetBlendColor( MicroCodeCommand command )
{
	u32 r	= N64COL_GETR(command.cmd1);
	u32 g	= N64COL_GETG(command.cmd1);
	u32 b	= N64COL_GETB(command.cmd1);
	u32 a	= N64COL_GETA(command.cmd1);

	use(r);
	use(g);
	use(b);

	DL_PF("    RGBA: %d %d %d %d", r, g, b, a);

	mpRenderer->SetAlphaRef(a);
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetPrimColor( MicroCodeCommand command )
{
	u32 m	= (command.cmd0>>8)&0xFF;
	u32 l	= (command.cmd0)&0xFF;
	u32 r	= N64COL_GETR(command.cmd1);
	u32 g	= N64COL_GETG(command.cmd1);
	u32 b	= N64COL_GETB(command.cmd1);
	u32 a	= N64COL_GETA(command.cmd1);

	use(m);
	use(l);

	DL_PF("    M:%d L:%d RGBA: %d %d %d %d", m, l, r, g, b, a);

	mpRenderer->SetPrimitiveColor( RGBA_MAKE(r, g, b, a) );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetEnvColor( MicroCodeCommand command )
{
	u32 r	= N64COL_GETR(command.cmd1);
	u32 g	= N64COL_GETG(command.cmd1);
	u32 b	= N64COL_GETB(command.cmd1);
	u32 a	= N64COL_GETA(command.cmd1);

	DL_PF("    RGBA: %d %d %d %d", r, g, b, a);

	mpRenderer->SetEnvColor( RGBA_MAKE(r, g, b, a) );
}
