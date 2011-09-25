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
#include "uCodes/Ucode.h"
#include "DLParser.h"
#include "PSPRenderer.h"
#include "PixelFormatN64.h"
#include "SysPSP/Graphics/PixelFormatPSP.h"
#include "RDP.h"
#include "RDPStateManager.h"
#include "DebugDisplayList.h"
#include "TextureCache.h"
#include "ConvertImage.h"			// Convert555ToRGBA
#include "Microcode.h"
#include "Ucodes/UcodeDefs.h"

#include "Utility/Profiler.h"
#include "Utility/IO.h"

#include "Graphics/GraphicsContext.h"
#include "Plugins/GraphicsPlugin.h"

#include "Debug/Dump.h"
#include "Debug/DBGConsole.h"

#include "Core/Memory.h"
#include "Core/ROM.h"
#include "Core/CPU.h"

#include "OSHLE/ultra_sptask.h"
#include "OSHLE/ultra_gbi.h"
#include "OSHLE/ultra_rcp.h"

#include "Test/BatchTest.h"

#include "ConfigOptions.h"

#include "../SysPSP/Utility/FastMemcpy.h"

#include <vector>

const u32	MAX_RAM_ADDRESS = (8*1024*1024);

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
const char *	gDisplayListRootPath = "DisplayLists";
const char *	gDisplayListDumpPathFormat = "dl%04d.txt";
#endif

//*****************************************************************************
//
//*****************************************************************************
#define RDP_NOIMPL_WARN(op)				DAEDALUS_DL_ERROR( op )
#define RDP_NOIMPL( op, cmd0, cmd1 )	DAEDALUS_DL_ERROR( "Not Implemented: %s 0x%08x 0x%08x", op, cmd0, cmd1 )

#define N64COL_GETR( col )		(u8((col) >> 24))
#define N64COL_GETG( col )		(u8((col) >> 16))
#define N64COL_GETB( col )		(u8((col) >>  8))
#define N64COL_GETA( col )		(u8((col)      ))

#define N64COL_GETR_F( col )	(N64COL_GETR(col) * (1.0f/255.0f))
#define N64COL_GETG_F( col )	(N64COL_GETG(col) * (1.0f/255.0f))
#define N64COL_GETB_F( col )	(N64COL_GETB(col) * (1.0f/255.0f))
#define N64COL_GETA_F( col )	(N64COL_GETA(col) * (1.0f/255.0f))

// Mask down to 0x003FFFFF?
#define RDPSegAddr(seg) ( (gSegments[((seg)>>24)&0x0F]&0x00ffffff) + ((seg)&0x00FFFFFF) )

static void RDP_Force_Matrix(u32 address);
void RDP_MoveMemViewport(u32 address);
void MatrixFromN64FixedPoint( Matrix4x4 & mat, u32 address );
static void DLParser_PopDL();
void DLParser_InitMicrocode( u32 code_base, u32 code_size, u32 data_base, u32 data_size );
void RDP_MoveMemLight(u32 light_idx, u32 address);
void DLParser_InitGeometryMode();

//*************************************************************************************
//
//*************************************************************************************

inline void 	FinishRDPJob()
{
	Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_DP);
	gCPUState.AddJob(CPU_CHECK_INTERRUPTS);
}

//*****************************************************************************
// Reads the next command from the display list, updates the PC.
//*****************************************************************************
inline void	DLParser_FetchNextCommand( MicroCodeCommand * p_command )
{
	// Current PC is the last value on the stack
	u32			pc( gDlistStack[gDlistStackPointer].pc );

	*p_command = *(MicroCodeCommand*)&g_pu32RamBase[(pc>>2)];

	gDlistStack[gDlistStackPointer].pc += 8;

}
//*************************************************************************************
//
//*************************************************************************************
u32 gRDPHalf1 = 0;
u32 gRDPFrame = 0;
u32 gAuxAddr = 0;
u32 gGeometryMode = 0;
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

struct RDP_Scissor
{
	u32 left, top, right, bottom;
};

enum CycleType
{
	CYCLE_1CYCLE = 0,		// Please keep in this order - matches RDP
	CYCLE_2CYCLE,
	CYCLE_COPY,
	CYCLE_FILL,
};

bool bIsOffScreen = false;
u32	gSegments[16];
static RDP_Scissor scissors;
static N64Light  g_N64Lights[16];	//Conker uses more than 8
SImageDescriptor g_TI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };
SImageDescriptor g_CI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };
SImageDescriptor g_DI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };

DListStack	gDlistStack[MAX_DL_STACK_SIZE];
s32			gDlistStackPointer = -1;

MicroCodeInstruction gInstructionLookupCustom[256];
const MicroCodeInstruction *gUcode = gInstructionLookup[0];

#if defined(DAEDALUS_DEBUG_DISPLAYLIST) || defined(DAEDALUS_ENABLE_PROFILING)
u32 gucode_ver=0;	//Need this global ucode version to get correct ucode names
#endif

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//////////////////////////////////////////////////////////
//                      Dumping                         //
//////////////////////////////////////////////////////////
static bool gDumpNextDisplayList = false;
FILE * gDisplayListFile = NULL;

//////////////////////////////////////////////////////////
//                      Debug vars                      //
//////////////////////////////////////////////////////////
void DLParser_DumpVtxInfo(u32 address, u32 v0_idx, u32 num_verts);

u32			gNumDListsCulled;
u32			gNumVertices;
u32			gNunRectsClipped;
static u32	gCurrentInstructionCount = 0;			// Used for debugging display lists
u32			gTotalInstructionCount = 0;
static u32	gInstructionCountLimit = UNLIMITED_INSTRUCTION_COUNT;

#define SCISSOR_RECT( x0, y0, x1, y1 ) \
	if( x0 >= scissors.right || y0 >= scissors.bottom ||  \
		x1 < scissors.left || y1 < scissors.top ) \
	{ \
		++gNunRectsClipped; \
		return; \
	}
#else
#define SCISSOR_RECT( x0, y0, x1, y1 ) \
	if( x0 >= scissors.right || y0 >= scissors.bottom ||  \
		x1 < scissors.left || y1 < scissors.top ) \
	{ \
		return; \
	}
#endif

static bool gFirstCall = true;	// Used to keep track of when we're processing the first display list

u32 gAmbientLightIdx = 0;
u32 gTextureTile = 0;
u32 gTextureLevel = 0;

u32 gFillColor		= 0xFFFFFFFF;

u32 gVertexStride;
 
#include "uCodes/Ucode_GBI0.h"
#include "uCodes/Ucode_GBI1.h"
#include "uCodes/Ucode_GBI2.h"
#include "uCodes/Ucode_DKR.h"
#include "uCodes/Ucode_GE.h"
#include "uCodes/Ucode_PD.h"
#include "uCodes/Ucode_Conker.h"
#include "uCodes/Ucode_LL.h"
#include "uCodes/Ucode_WRUS.h"
#include "uCodes/Ucode_SOTE.h"
#include "uCodes/Ucode_Sprite2D.h"
#include "uCodes/Ucode_S2DEX.h"

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                      Strings                         //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
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
#endif

//*****************************************************************************
//
//*****************************************************************************
bool DLParser_Initialise()
{

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	gDumpNextDisplayList = false;
	gDisplayListFile = NULL;
#endif

	gFirstCall = true;

	// Reset scissor to default
	//
	scissors.top = 0;
	scissors.left = 0;
	scissors.right = 320;
	scissors.bottom = 240;

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

//*****************************************************************************
//
//*****************************************************************************
static void	DLParser_DumpTaskInfo( const OSTask * pTask )
{
	DL_PF( "Task:         %08x",      pTask->t.type  );
	DL_PF( "Flags:        %08x",      pTask->t.flags  );
	DL_PF( "BootCode:     %08x", (u32)pTask->t.ucode_boot  );
	DL_PF( "BootCodeSize: %08x",      pTask->t.ucode_boot_size  );

	DL_PF( "uCode:        %08x", (u32)pTask->t.ucode );
	DL_PF( "uCodeSize:    %08x",      pTask->t.ucode_size );
	DL_PF( "uCodeData:    %08x", (u32)pTask->t.ucode_data );
	DL_PF( "uCodeDataSize:%08x",      pTask->t.ucode_data_size );

	DL_PF( "Stack:        %08x", (u32)pTask->t.dram_stack );
	DL_PF( "StackS:       %08x",      pTask->t.dram_stack_size );
	DL_PF( "Output:       %08x", (u32)pTask->t.output_buff );
	DL_PF( "OutputS:      %08x", (u32)pTask->t.output_buff_size );

	DL_PF( "Data( PC ):   %08x", (u32)pTask->t.data_ptr );
	DL_PF( "DataSize:     %08x",      pTask->t.data_size );
	DL_PF( "YieldData:    %08x", (u32)pTask->t.yield_data_ptr );
	DL_PF( "YieldDataSize:%08x",      pTask->t.yield_data_size );
}

//*************************************************************************************
// 
//*************************************************************************************
static void HandleDumpDisplayList( OSTask * pTask )
{
	if (gDumpNextDisplayList)
	{
		DBGConsole_Msg( 0, "Dumping display list" );
		static u32 count = 0;

		char szFilePath[MAX_PATH+1];
		char szFileName[MAX_PATH+1];
		char szDumpDir[MAX_PATH+1];

		IO::Path::Combine(szDumpDir, g_ROM.settings.GameName.c_str(), gDisplayListRootPath);
	
		Dump_GetDumpDirectory(szFilePath, szDumpDir);

		sprintf(szFileName, "dl%04d.txt", count++);

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
}
#endif

//*****************************************************************************
//
//*****************************************************************************
/*
void DLParser_PushDisplayList( const DList & dl )
{
	gDisplayListStack.push_back( dl );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if ( gDisplayListStack.size() > 30 )
	{
		// Stack is too deep - probably an error
		DAEDALUS_DL_ERROR( "Stack is over 30 - something is probably wrong\n" );
	}
#endif
	DL_PF("    Pushing DisplayList 0x%08x", dl.addr);
	DL_PF(" ");
	DL_PF("\\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/");
	DL_PF("############################################");
}
*/
//*****************************************************************************
//
//*****************************************************************************
/*
void DLParser_CallDisplayList( const DList & dl )
{
	gDisplayListStack.back() = dl;

	DL_PF("    Jumping to DisplayList 0x%08x", dl.addr);
	DL_PF(" ");
	DL_PF("\\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/");
	DL_PF("############################################");
}
*/
//*****************************************************************************
//
//*****************************************************************************
static void DLParser_PopDL()
{
	DL_PF("Returning from DisplayList: level=%d", gDlistStackPointer+1);
	DL_PF("############################################");
	DL_PF("/\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\");
	DL_PF(" ");

	gDlistStackPointer--;
}

#define UCODE_SIZE 1024 

#define SetCommand( cmd, func )					\
		gInstructionLookupCustom[ cmd ] = func;

#define SetCustom( ucode, stride, size )			\
			gVertexStride = stride;					\
			gUcode = gInstructionLookupCustom;		\
			memcpy( &gInstructionLookupCustom, 		\
				    &gInstructionLookup[ ucode ], size );

#define SetNormal( ucode, stride )			\
			gVertexStride = stride;					\
			gUcode = gInstructionLookup[ ucode ];

//*************************************************************************************
// 
//*************************************************************************************
void DLParser_SetUcode( u32 ucode )
{	
	// This is the multiplier applied to vertex indices. 
	const u32 stride[] =
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

	// This a normal ucode, just retrive the correct ucode table
	if( !current.custom )
	{
		SetNormal( ucode, stride[ ucode ] );
		return;
	}

	//
	// Reserved for custom ucodes
	//

	const u32 info[] =
	{
		0,		// Modified uCode 0 - RSP SW 2.0D EXT
		0,		// Modified uCode 0 - RSP SW 2.0 Diddy
		1,		// Modified uCode 1 - F3DEX Last Legion
		0,		// Modified uCode 0 - RSP SW 2.0D EXT
		0,		// Modified uCode 0 - RSP SW 2.0X
		2,		// Modified uCode 2:  F3DEXBGxx Conker
		0,		// Modified uCode 0 - Unknown
	};

	// First build array based from a normal uCode table
	SetCustom( info[ ucode-MAX_UCODE ], stride[ ucode ], UCODE_SIZE);

	// Now let's patch it, to create our custom ucode ;)
	switch( ucode )
	{
		case GBI_GE:
			SetCommand( G_GBI1_RDPHALF_1, DLParser_RDPHalf1_GoldenEye );
			break;
		case GBI_WR:
			SetCommand( G_GBI1_VTX,  DLParser_GBI0_Vtx_WRUS );
			SetCommand( G_GBI1_TRI2, DLParser_Nothing ); // Just in case
			break;
		case GBI_SE:
			SetCommand( G_GBI1_VTX, DLParser_GBI0_Vtx_SOTE );
			SetCommand( G_GBI1_DL,  DLParser_GBI0_DL_SOTE );
			break;
		case GBI_LL:
			SetCommand( 0x80,		   DLParser_RSP_Last_Legion_0x80 );
			SetCommand( 0x00,		   DLParser_RSP_Last_Legion_0x00 );
			SetCommand( G_RDP_TEXRECT, DLParser_TexRect_Last_Legion );
			break;
		case GBI_PD:
			SetCommand( G_GBI1_VTX,		  RSP_Vtx_PD );
			SetCommand( G_GBI1_RESERVED2, RSP_Set_Vtx_CI_PD );
			SetCommand( G_GBI1_RDPHALF_1, DLParser_RDPHalf1_GoldenEye );
			break;
		case GBI_DKR:
			SetCommand( G_GBI1_MTX,		  DLParser_Mtx_DKR );
			SetCommand( G_GBI1_VTX,		  DLParser_GBI0_Vtx_DKR );
			SetCommand( G_GBI1_RESERVED1, DLParser_DMA_Tri_DKR );
			SetCommand( G_GBI1_RESERVED2, DLParser_DLInMem );
			SetCommand( G_GBI1_MOVEWORD,  DLParser_MoveWord_DKR );
			SetCommand( G_GBI1_TRI1,	  DLParser_Set_Addr_DKR );
			break;
		case GBI_CONKER:
			SetCommand( 0x01, RSP_Vtx_Conker );
			SetCommand( 0x10, DLParser_GBI2_Conker );
			SetCommand( 0x11, DLParser_GBI2_Conker );
			SetCommand( 0x12, DLParser_GBI2_Conker );
			SetCommand( 0x13, DLParser_GBI2_Conker );
			SetCommand( 0x14, DLParser_GBI2_Conker );
			SetCommand( 0x15, DLParser_GBI2_Conker );
			SetCommand( 0x16, DLParser_GBI2_Conker );
			SetCommand( 0x17, DLParser_GBI2_Conker );
			SetCommand( 0x18, DLParser_GBI2_Conker );
			SetCommand( 0x19, DLParser_GBI2_Conker );
			SetCommand( 0x1a, DLParser_GBI2_Conker );
			SetCommand( 0x1b, DLParser_GBI2_Conker );
			SetCommand( 0x1c, DLParser_GBI2_Conker );
			SetCommand( 0x1d, DLParser_GBI2_Conker );
			SetCommand( 0x1e, DLParser_GBI2_Conker );
			SetCommand( 0x1f, DLParser_GBI2_Conker );
			SetCommand( 0xdb, RSP_MoveWord_Conker );
			SetCommand( 0xdc, RSP_MoveMem_Conker );
			break;
	}
}
//*****************************************************************************
// 
//*****************************************************************************
void DLParser_InitMicrocode( u32 code_base, u32 code_size, u32 data_base, u32 data_size )
{
	// Start ucode detector
	u32 ucode = GBIMicrocode_DetectVersion( code_base, code_size, data_base, data_size );
	
	// Store useful information about this ucode for caching purpose
	current.code_base = code_base;
	current.ucode	  = ucode; 
	current.custom	  = ( ucode > GBI_1_S2DEX );

	// Set up ucode table, patch custom ucodes, set up vtx multiplier etc
	DLParser_SetUcode( ucode );

	//if ucode version is other than 0,1 or 2 then default to 2 (with potentially non valid function names) 
	//
#if defined(DAEDALUS_DEBUG_DISPLAYLIST) || defined(DAEDALUS_ENABLE_PROFILING)
	switch (ucode)
	{
		case GBI_0:	//GBI0
		case GBI_1:	//GBI1
		case GBI_2:	//GBI2	
			gucode_ver = ucode;
			break;
		case GBI_DKR:	//DKR	
		case GBI_PD:	//PD	
			gucode_ver = 0;
			break;
		case GBI_CONKER:	//Conker
			gucode_ver = 3;
			break;
		default:	//Default to 2 otherwise
			gucode_ver = 2;
			break;
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
//if ucode verion is other than 0,1 or 2 then default to 0 (with non valid function names) //Corn 
#ifdef DAEDALUS_ENABLE_PROFILING
SProfileItemHandle * gpProfileItemHandles[ 256 ];

#define PROFILE_DL_CMD( cmd )								\
	if(gpProfileItemHandles[ (cmd) ] == NULL)				\
	{														\
		gpProfileItemHandles[ (cmd) ] = new SProfileItemHandle( CProfiler::Get()->AddItem( gInstructionName[ gucode_ver ][ cmd ] );		\
	}														\
	CAutoProfile		_auto_profile( *gpProfileItemHandles[ (cmd) ] )

#else 

#define PROFILE_DL_CMD( cmd )		do { } while(0)

#endif

//*****************************************************************************
//	Process the entire display list in one go
//*****************************************************************************
static void	DLParser_ProcessDList()
{
	MicroCodeCommand command;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	//Check if address is outside legal RDRAM
	u32			pc( gDlistStack[gDlistStackPointer].pc );

	if ( pc > MAX_RAM_ADDRESS )
	{
		DBGConsole_Msg(0, "Display list PC is out of range: 0x%08x", pc );
		return;
	}
#endif

	while(gDlistStackPointer >= 0)
	{
		DLParser_FetchNextCommand( &command );

		// Note: if instruction (gCurrentInstructionCount) is zero (aka dlist was culled) and we happen to launch the dlist debugger
		// it won't work since that display list was culled and nothing will be shown
		// I don't think there's a way to "fix" that without sacrficing performance or adding a hack for debug builds -Salvy
		//
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		//use the gInstructionName table for fecthing names.
		//we use the table as is for GBI0, GBI1 and GBI2
		//we fallback to GBI0 for custom ucodes (ucode_ver>2)
		DL_PF("[%05d] 0x%08x: %08x %08x %-10s", gCurrentInstructionCount, pc, command.inst.cmd0, command.inst.cmd1, gInstructionName[ gucode_ver ][command.inst.cmd ]);
		gCurrentInstructionCount++;

		if( gInstructionCountLimit != UNLIMITED_INSTRUCTION_COUNT )
		{
			if( gCurrentInstructionCount >= gInstructionCountLimit )
			{
				return;
			}
		}
#endif
		//if(!(gCurrentInstructionCount % 1024)) printf("%d\n",gCurrentInstructionCount);

		PROFILE_DL_CMD( command.inst.cmd );

		gUcode[ command.inst.cmd ]( command ); 

		// Check limit
		if ( --gDlistStack[gDlistStackPointer].countdown < 0 && gDlistStackPointer >= 0)
		{
			DL_PF("**EndDLInMem");
			gDlistStackPointer--;
		}

	}
}
//*****************************************************************************
//
//*****************************************************************************
void DLParser_Process()
{
	DAEDALUS_PROFILE( "DLParser_Process" );

	if ( !CGraphicsContext::Get()->IsInitialised() || !PSPRenderer::IsAvailable() )
	{
		return;
	}

	// Shut down the debug console when we start rendering
	// TODO: Clear the front/backbuffer the first time this function is called
	// to remove any stuff lingering on the screen.
	if(gFirstCall)
	{
#ifdef DAEDALUS_DEBUG_CONSOLE
		CDebugConsole::Get()->EnableConsole( false );
#endif
		CGraphicsContext::Get()->ClearAllSurfaces();

		gFirstCall = false;
	}

	// Update Screen only when something is drawn, otherwise several games ex Army Men will flash or shake.
	// Update Screen earlier, otherwise several games like ex Mario won't work.
	//
	if( g_ROM.GameHacks != CHAMELEON_TWIST_2 ) gGraphicsPlugin->UpdateScreen();
	
	OSTask * pTask = (OSTask *)(g_pu8SpMemBase + 0x0FC0);
	u32 code_base = (u32)pTask->t.ucode & 0x1fffffff;
	u32 code_size = pTask->t.ucode_size;
	u32 data_base = (u32)pTask->t.ucode_data & 0x1fffffff;
	u32 data_size = pTask->t.ucode_data_size;
	
	if ( current.code_base != code_base )
	{
		DLParser_InitMicrocode( code_base, code_size, data_base, data_size );
	}

	//
	// Not sure what to init this with. We should probably read it from the dmem
	//
	gRDPOtherMode.L = 0x00500001;
	gRDPOtherMode.H = 0;

	gRDPFrame++;

	CTextureCache::Get()->PurgeOldTextures();

	// Initialise stack
	gDlistStackPointer=0;
	gDlistStack[gDlistStackPointer].pc = (u32)pTask->t.data_ptr;
	gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;

	gRDPStateManager.Reset();

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	gTotalInstructionCount = 0;
	gCurrentInstructionCount = 0;
	gNumDListsCulled = 0;
	gNumVertices = 0;
	gNunRectsClipped =0;
	//
	// Prepare to dump this displaylist, if necessary
	//
	HandleDumpDisplayList( pTask );
#endif

	DL_PF("DP: Firing up RDP!");

	extern bool gFrameskipActive;
	if(!gFrameskipActive)
	{
		PSPRenderer::Get()->SetVIScales();
		PSPRenderer::Get()->Reset();
		PSPRenderer::Get()->BeginScene();
		DLParser_ProcessDList();
		PSPRenderer::Get()->EndScene();
	}

	// Hack for Chameleon Twist 2, only works if screen is update at last
	//
	if( g_ROM.GameHacks == CHAMELEON_TWIST_2 ) gGraphicsPlugin->UpdateScreen();

	// Do this regardless!
	FinishRDPJob();

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		fclose( gDisplayListFile );
		gDisplayListFile = NULL;
	}

	gTotalInstructionCount = gCurrentInstructionCount;

	//printf("%d\n", gTotalInstructionCount);
	//if(gTotalInstructionCount == 6) ThreadSleepMs(500);
#endif

#ifdef DAEDALUS_BATCH_TEST_ENABLED
	CBatchTestEventHandler * handler( BatchTest_GetHandler() );
	if( handler )
		handler->OnDisplayListComplete();
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void RDP_MoveMemLight(u32 light_idx, u32 address)
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if( light_idx >= 16 )
	{
		DBGConsole_Msg(0, "Warning: invalid light # = %d", light_idx);
		return;
	}
#endif
	s8 * pcBase = g_ps8RamBase + address;
	u32 * pdwBase = (u32 *)pcBase;

	g_N64Lights[light_idx].Colour     = pdwBase[0];
	g_N64Lights[light_idx].ColourCopy = pdwBase[1];
	g_N64Lights[light_idx].x			= f32(pcBase[8 ^ 0x3]);
	g_N64Lights[light_idx].y			= f32(pcBase[9 ^ 0x3]);
	g_N64Lights[light_idx].z			= f32(pcBase[10 ^ 0x3]);
					
	DL_PF("       RGBA: 0x%08x, RGBACopy: 0x%08x, x: %f, y: %f, z: %f", 
		g_N64Lights[light_idx].Colour,
		g_N64Lights[light_idx].ColourCopy,
		g_N64Lights[light_idx].x,
		g_N64Lights[light_idx].y,
		g_N64Lights[light_idx].z);

	if (light_idx == gAmbientLightIdx)
	{
		DL_PF("      (Ambient Light)");

		u32 n64col( g_N64Lights[light_idx].Colour );

		PSPRenderer::Get()->SetAmbientLight( v4( N64COL_GETR_F(n64col), N64COL_GETG_F(n64col), N64COL_GETB_F(n64col), 1.0f ) );
	}
	else
	{
		
		DL_PF("      (Normal Light)");

		PSPRenderer::Get()->SetLightCol(light_idx, g_N64Lights[light_idx].Colour);
		if (pdwBase[2] == 0)	// Direction is 0!
		{
			DL_PF("      Light is invalid");
		}
		else
		{
			PSPRenderer::Get()->SetLightDirection(light_idx, 
										  g_N64Lights[light_idx].x,
										  g_N64Lights[light_idx].y,
										  g_N64Lights[light_idx].z);
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
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if( address+16 >= MAX_RAM_ADDRESS )
	{
		DBGConsole_Msg(0, "MoveMem Viewport, invalid memory");
		return;
	}
#endif
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

	v3 vec_scale( scale[0] * 0.25f, scale[1] * 0.25f, scale[2] * 0.25f );
	v3 vec_trans( trans[0] * 0.25f, trans[1] * 0.25f, trans[2] * 0.25f );

	PSPRenderer::Get()->SetN64Viewport( vec_scale, vec_trans );

	DL_PF("        Scale: %d %d %d %d", scale[0], scale[1], scale[2], scale[3]);
	DL_PF("        Trans: %d %d %d %d", trans[0], trans[1], trans[2], trans[3]);
}

//*****************************************************************************
//
//*****************************************************************************


//Nintro64 uses Sprite2d 
void DLParser_Nothing( MicroCodeCommand command )
{
	DAEDALUS_DL_ERROR( "RDP Command %08x Does not exist...", command.inst.cmd0 );

	// Terminate!
	//	DBGConsole_Msg(0, "Warning, DL cut short with unknown command: 0x%08x 0x%08x", command.inst.cmd0, command.inst.cmd1);
	DLParser_PopDL();

}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_SpNoop( MicroCodeCommand command )
{
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*****************************************************************************
//
//*****************************************************************************
#define DL_UNIMPLEMENTED_ERROR( msg )			\
{												\
	static bool shown = false;					\
	if (!shown )								\
	{											\
		DAEDALUS_DL_ERROR( "%s: %08x %08x", (msg), command.inst.cmd0, command.inst.cmd1 );				\
		shown = true;							\
	}											\
}
#endif
//*****************************************************************************
//
//*****************************************************************************

void DLParser_GBI2_DMA_IO( MicroCodeCommand command )
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DL_PF( "~*Not Implemented (G_DMA_IO in GBI 2)" );
	DL_UNIMPLEMENTED_ERROR( "G_DMA_IO" );
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_Reserved( MicroCodeCommand command )
{	
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DL_PF( "~*Not Implemented" );

	// Spiderman
	static bool warned = false;

	if (!warned)
	{
		RDP_NOIMPL("RDP: Reserved (0x%08x 0x%08x)", command.inst.cmd0, command.inst.cmd1);
		warned = true;
	}
#endif	
	// Not implemented!
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
	DL_PF( "	SetKeyGB (Ignored)" );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetKeyR( MicroCodeCommand command )
{
	DL_PF( "	SetKeyR (Ignored)" );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetConvert( MicroCodeCommand command )
{
	DL_PF( "	SetConvert (Ignored)" );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetPrimDepth( MicroCodeCommand command )
{
	DL_PF("SetPrimDepth: 0x%08x 0x%08x - z: 0x%04x dz: 0x%04x", 
		   command.inst.cmd0, command.inst.cmd1, command.primdepth.z, command.primdepth.dz);	
	
	PSPRenderer::Get()->SetPrimitiveDepth( command.primdepth.z );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_RDPSetOtherMode( MicroCodeCommand command )
{
	DL_PF( "      RDPSetOtherMode: 0x%08x 0x%08x", command.inst.cmd0, command.inst.cmd1 );

	gRDPOtherMode.H = command.inst.cmd0;
	gRDPOtherMode.L = command.inst.cmd1;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	RDP_SetOtherMode( gRDPOtherMode.H, gRDPOtherMode.L );
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_RDPHalf_Cont( MicroCodeCommand command )
{
	//DBGConsole_Msg( 0, "Unexpected RDPHalf_Cont: %08x %08x", command.inst.cmd0, command.inst.cmd1 );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_RDPHalf_2( MicroCodeCommand command )
{
//	DBGConsole_Msg( 0, "Unexpected RDPHalf_2: %08x %08x", command.inst.cmd0, command.inst.cmd1 );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_GBI1_RDPHalf_1( MicroCodeCommand command )
{
	gRDPHalf1 = command.inst.cmd1;
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
	// This is done after DLIST processing anyway
	//FinishRDPJob();

	/*DL_PF("FullSync: (Generating Interrupt)");*/
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_InitGeometryMode()
{
	// CULL_BACK has priority, Fixes Mortal Kombat 4
	bool bCullFront         = (gGeometryMode & G_CULL_FRONT)		? true : false;
	bool bCullBack          = (gGeometryMode & G_CULL_BACK)			? true : false;
	PSPRenderer::Get()->SetCullMode(bCullFront, bCullBack);

	bool bShade				= (gGeometryMode & G_SHADE)				? true : false;
	PSPRenderer::Get()->SetSmooth( bShade );

	bool bShadeSmooth       = (gGeometryMode & G_SHADING_SMOOTH)	? true : false;
	PSPRenderer::Get()->SetSmoothShade( bShadeSmooth );

	bool bFog				= (gGeometryMode & G_FOG)				? true : false;
	PSPRenderer::Get()->SetFogEnable( bFog );

	bool bTextureGen        = (gGeometryMode & G_TEXTURE_GEN)		? true : false;
	PSPRenderer::Get()->SetTextureGen(bTextureGen);

	bool bTextureGenLin        = (gGeometryMode & G_TEXTURE_GEN_LINEAR)			? true : false;
	PSPRenderer::Get()->SetTextureGenLin( bTextureGenLin );

	bool bLighting			= (gGeometryMode & G_LIGHTING)			? true : false;
	PSPRenderer::Get()->SetLighting( bLighting );

	bool bZBuffer           = (gGeometryMode & G_ZBUFFER)			? true : false;
	PSPRenderer::Get()->ZBufferEnable( bZBuffer );

	DL_PF("  ZBuffer %s", bZBuffer ? "On" : "Off");
	DL_PF("  Culling %s", bCullBack ? "Back face" : bCullFront ? "Front face" : "Off");
	DL_PF("  Shade %s", bShade ? "On" : "Off");
	DL_PF("  Smooth Shading %s", bShadeSmooth ? "On" : "Off");
	DL_PF("  Lighting %s", bLighting ? "On" : "Off");
	DL_PF("  Texture %s", (gGeometryMode & G_TEXTURE_ENABLE) ? "On" : "Off");
	DL_PF("  Texture Gen %s", bTextureGen ? "On" : "Off");
	DL_PF("  Texture Gen Linear %s", bTextureGenLin ? "On" : "Off");
	DL_PF("  Fog %s", bFog ? "On" : "Off");
	DL_PF("  LOD %s", (gGeometryMode & G_LOD) ? "On" : "Off");
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*****************************************************************************
//
//*****************************************************************************
void DLParser_DumpVtxInfo(u32 address, u32 v0_idx, u32 num_verts)
{
	if (gDisplayListFile != NULL)
	{
		s8 *pcSrc = (s8 *)(g_pu8RamBase + address);
		s16 *psSrc = (s16 *)(g_pu8RamBase + address);

		for ( u32 idx = v0_idx; idx < v0_idx + num_verts; idx++ )
		{
			f32 x = f32(psSrc[0^0x1]);
			f32 y = f32(psSrc[1^0x1]);
			f32 z = f32(psSrc[2^0x1]);

			u16 wFlags = u16(PSPRenderer::Get()->GetVtxFlags( idx )); //(u16)psSrc[3^0x1];

			u8 a = pcSrc[12^0x3];
			u8 b = pcSrc[13^0x3];
			u8 c = pcSrc[14^0x3];
			u8 d = pcSrc[15^0x3];
			
			s16 nTU = psSrc[4^0x1];
			s16 nTV = psSrc[5^0x1];

			f32 tu = f32(nTU) * (1.0f / 32.0f);
			f32 tv = f32(nTV) * (1.0f / 32.0f);

			const v4 & t = PSPRenderer::Get()->GetTransformedVtxPos( idx );
			const v4 & p = PSPRenderer::Get()->GetProjectedVtxPos( idx );

			psSrc += 8;			// Increase by 16 bytes
			pcSrc += 16;

			DL_PF("   #%02d Flags: 0x%04x Pos:{% 6f,% 6f,% 6f} Tex:{%+7.2f,%+7.2f} Extra: %02x %02x %02x %02x Tran:{% 6f,% 6f,% 6f,% 6f} Proj:{% 6f,% 6f,% 6f,% 6f}",
				idx, wFlags, x, y, z, tu, tv, a, b, c, d, t.x, t.y, t.z, t.w, p.x/p.w, p.y/p.w, p.z/p.w, p.w);
		}
	}
}
#endif

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetScissor( MicroCodeCommand command )
{
	// The coords are all in 8:2 fixed point
	// Set up scissoring zone, we'll use it to scissor other stuff ex Texrect
	//
	scissors.left    = command.scissor.x0>> 2;
	scissors.top	 = command.scissor.y0>> 2;
	scissors.right   = command.scissor.x1>> 2;
	scissors.bottom  = command.scissor.y1>> 2;

	// Hack to correct Super Bowling's right screen, left screen needs fb emulation
	if ( g_ROM.GameHacks == SUPER_BOWLING && g_CI.Address%0x100 != 0 )
	{
		u32 addr = RDPSegAddr(command.inst.cmd1);

		// right half screen
		RDP_MoveMemViewport( addr );
	}

	DL_PF("    x0=%d y0=%d x1=%d y1=%d mode=%d", scissors.left, scissors.top, scissors.right, scissors.bottom, command.scissor.mode);

	// Set the cliprect now...
	if ( scissors.left < scissors.right && scissors.top < scissors.bottom )
	{
		PSPRenderer::Get()->SetScissor( scissors.left, scissors.top, scissors.right, scissors.bottom );
	}
}
//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetTile( MicroCodeCommand command )
{
	RDP_Tile tile;
	tile.cmd0 = command.inst.cmd0;
	tile.cmd1 = command.inst.cmd1;
	
	gRDPStateManager.SetTile( tile );

	DL_PF( "    Tile:%d  Fmt: %s/%s Line:%d TMem:0x%04x Palette:%d", tile.tile_idx, gFormatNames[tile.format], gSizeNames[tile.size], tile.line, tile.tmem, tile.palette);
	DL_PF( "         S: Clamp:%s Mirror:%s Mask:0x%x Shift:0x%x", gOnOffNames[tile.clamp_s],gOnOffNames[tile.mirror_s], tile.mask_s, tile.shift_s );
	DL_PF( "         T: Clamp:%s Mirror:%s Mask:0x%x Shift:0x%x", gOnOffNames[tile.clamp_t],gOnOffNames[tile.mirror_t], tile.mask_t, tile.shift_t );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetTileSize( MicroCodeCommand command )
{
	RDP_TileSize tile;
	tile.cmd0 = command.inst.cmd0;
	tile.cmd1 = command.inst.cmd1;

	DL_PF("    Tile:%d (%d,%d) -> (%d,%d) [%d x %d]",
		tile.tile_idx, tile.left/4, tile.top/4,
		        tile.right/4, tile.bottom/4,
				((tile.right/4) - (tile.left/4)) + 1,
				((tile.bottom/4) - (tile.top/4)) + 1);

	gRDPStateManager.SetTileSize( tile );
}


//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetTImg( MicroCodeCommand command )
{
	g_TI.Format		= command.img.fmt;
	g_TI.Size		= command.img.siz;
	g_TI.Width		= command.img.width + 1;
	g_TI.Address	= RDPSegAddr(command.img.addr);
	//g_TI.bpl		= g_TI.Width << g_TI.Size >> 1; // Do we need to handle?

	DL_PF("    Image: 0x%08x Fmt: %s/%s Width: %d (Pitch: %d) Bytes/line:%d", g_TI.Address, gFormatNames[g_TI.Format], gSizeNames[g_TI.Size], g_TI.Width, g_TI.GetPitch(), g_TI.Width << g_TI.Size >> 1 );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_LoadBlock( MicroCodeCommand command )
{
	u32 uls			= command.loadtile.sl;
	u32 ult			= command.loadtile.tl;
	u32 tile_idx	= command.loadtile.tile;
	//u32 lrs			= command.loadtile.sh;		// Number of bytes-1
	u32 dxt			= command.loadtile.th;		// 1.11 fixed point

	bool	swapped = (dxt) ? false : true;

	u32		src_offset = g_TI.Address + ult * (g_TI.Width << g_TI.Size >> 1) + (uls << g_TI.Size >> 1);

	DL_PF("    Tile:%d (%d,%d - %d) DXT:0x%04x = %d Bytes => %d pixels/line", tile_idx, uls, ult, command.loadtile.sh, dxt, (g_TI.Width << g_TI.Size >> 1), bytes2pixels( (g_TI.Width << g_TI.Size >> 1), g_TI.Size ));
	DL_PF("    Offset: 0x%08x", src_offset);

	gRDPStateManager.LoadBlock( tile_idx, src_offset, swapped );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_LoadTile( MicroCodeCommand command )
{
	RDP_TileSize tile;
	tile.cmd0 = command.inst.cmd0;
	tile.cmd1 = command.inst.cmd1;
	
	DL_PF("    Tile:%d (%d,%d) -> (%d,%d) [%d x %d]",	tile.tile_idx, tile.left/4, tile.top/4, tile.right/4 + 1, tile.bottom / 4 + 1, (tile.right - tile.left)/4+1, (tile.bottom - tile.top)/4+1);
	DL_PF("    Offset: 0x%08x",							g_TI.GetOffset( tile.left, tile.top ) );

	gRDPStateManager.LoadTile( tile );
}

//*****************************************************************************
// Load data into the tlut
//*****************************************************************************
void DLParser_LoadTLut( MicroCodeCommand command )
{
	u32 uls     = command.loadtile.sl >> 2;
	u32 ult		= command.loadtile.tl >> 2;
	u32 tile_idx= command.loadtile.tile;
	u32 lrs     = command.loadtile.sh >> 2;

	//This corresponds to the number of palette entries (16 or 256)
	//Seems partial load of palette is allowed -> count != 16 or 256 (MM, SSB, Starfox64, MRC) //Corn
	u32 count = (lrs - uls) + 1;
	use(count);

	// Format is always 16bpp - RGBA16 or IA16:
	u32 offset = (uls + ult * g_TI.Width) << 1;

	const RDP_Tile & rdp_tile( gRDPStateManager.GetTile( tile_idx ) );

#ifndef DAEDALUS_TMEM
	//Store address of PAL (assuming PAL is only stored in upper half of TMEM) //Corn
	gTextureMemory[ rdp_tile.tmem & 0xFF ] = (u32*)&g_pu8RamBase[ g_TI.Address + offset ];
#else
	//Copy PAL to the PAL memory
	u16 * p_source = (u16*)&g_pu8RamBase[ g_TI.Address + offset ];
	u16 * p_dest   = (u16*)&gTextureMemory[ rdp_tile.tmem << 1 ];

	//printf("Addr %08X : TMEM %03X : Tile %d : PAL %d : Offset %d\n",g_TI.Address + offset, tmem, tile_idx, count, offset); 

	memcpy_vfpu_BE(p_dest, p_source, count << 1);

	//for (u32 i=0; i<count; i++)
	//{
	//	p_dest[ i ] = p_source[ i ];
	//	//if(count & 0x10) printf("%04X ",p_source[ i ]);
	//}
	//if(count & 0x10) printf("\n");
#endif

	// Format is always 16bpp - RGBA16 or IA16:
	// I've no idea why these two are added - seems to work for 007!

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	u32 lrt = command.loadtile.th >> 2;

	if (gDisplayListFile != NULL)
	{
		char str[300] = "";
		char item[300];
		u16 * pBase = (u16 *)(g_pu8RamBase + (g_TI.Address + offset));
		u32 i;


		DL_PF("    LoadTLut Addr:0x%08x Offset:0x%05x TMEM:0x%04x Tile:%d, (%d,%d) -> (%d,%d), Count %d", g_TI.Address, offset, rdp_tile.tmem,
			tile_idx, uls, ult, lrs, lrt, count);
		// This is sometimes wrong (in 007) tlut fmt is set after 
		// tlut load, but before tile load
		DL_PF("    Fmt is %s", gTLUTTypeName[gRDPOtherMode.text_tlut]);

		for (i = 0; i < count; i++)
		{
			u16 wEntry = pBase[i ^ 0x1];

			if (i % 8 == 0)
			{
				DL_PF(str);

				// Clear
				sprintf(str, "%03d: ", i);
			}

			PixelFormats::N64::Pf5551	n64col( wEntry );
			PixelFormats::Psp::Pf8888	pspcol( PixelFormats::Psp::Pf8888::Make( n64col ) );

			sprintf(item, "0x%04x (0x%08x) ", n64col.Bits, pspcol.Bits );
			strcat(str, item);
		}
		DL_PF(str);
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_TexRect( MicroCodeCommand command )
{
	MicroCodeCommand command2;
	MicroCodeCommand command3;

	//
	// Fetch the next two instructions
	//
	DLParser_FetchNextCommand( &command2 );
	DLParser_FetchNextCommand( &command3 );

	RDP_TexRect tex_rect;
	tex_rect.cmd0 = command.inst.cmd0;
	tex_rect.cmd1 = command.inst.cmd1;
	tex_rect.cmd2 = command2.inst.cmd1;
	tex_rect.cmd3 = command3.inst.cmd1;

	
	/// Note this will break framebuffer effects.
	//
	if( bIsOffScreen )	return;

	// Do compare with integers saves CPU //Corn
	u32	x0 = tex_rect.x0 >> 2;
	u32	y0 = tex_rect.y0 >> 2;
	u32	x1 = tex_rect.x1 >> 2;
	u32	y1 = tex_rect.y1 >> 2;

	// Removes offscreen texrect, also fixes several glitches like in John Romero's Daikatana
	//
	SCISSOR_RECT( x0, y0, x1, y1 );

	//Not using floats here breaks GE 007 intro
	v2 d( tex_rect.dsdx / 1024.0f, tex_rect.dtdy / 1024.0f );
	v2 xy0( tex_rect.x0 / 4.0f, tex_rect.y0 / 4.0f );
	v2 xy1;
	v2 uv0( tex_rect.s / 32.0f, tex_rect.t / 32.0f );
	v2 uv1;

	//
	// In Fill/Copy mode the coordinates are inclusive (i.e. add 1.0f to the w/h)
	//
	switch ( gRDPOtherMode.cycle_type )
	{
		case CYCLE_COPY:
			d.x *= 0.25f;	// In copy mode 4 pixels are copied at once.
		case CYCLE_FILL:
			xy1.x = (tex_rect.x1 + 4) * 0.25f;
			xy1.y = (tex_rect.y1 + 4) * 0.25f;
			break;
		default:
			xy1.x = tex_rect.x1 * 0.25f;
			xy1.y = tex_rect.y1 * 0.25f;
			break;
	}

	uv1.x = uv0.x + d.x * ( xy1.x - xy0.x );
	uv1.y = uv0.y + d.y * ( xy1.y - xy0.y );

	DL_PF("    Screen(%.1f,%.1f) -> (%.1f,%.1f) Tile:%d", xy0.x, xy0.y, xy1.x, xy1.y, tex_rect.tile_idx);
	DL_PF("    Tex:(%#5.3f,%#5.3f) -> (%#5.3f,%#5.3f) (DSDX:%#5f DTDY:%#5f)", uv0.x, uv0.y, uv1.x, uv1.y, d.x, d.y);

	PSPRenderer::Get()->TexRect( tex_rect.tile_idx, xy0, xy1, uv0, uv1 );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_TexRectFlip( MicroCodeCommand command )
{ 
	MicroCodeCommand command2;
	MicroCodeCommand command3;

	//
	// Fetch the next two instructions
	//
	DLParser_FetchNextCommand( &command2 );
	DLParser_FetchNextCommand( &command3 );

	RDP_TexRect tex_rect;
	tex_rect.cmd0 = command.inst.cmd0;
	tex_rect.cmd1 = command.inst.cmd1;
	tex_rect.cmd2 = command2.inst.cmd1;
	tex_rect.cmd3 = command3.inst.cmd1;

	v2 d( tex_rect.dsdx / 1024.0f, tex_rect.dtdy / 1024.0f );
	v2 xy0( tex_rect.x0 / 4.0f, tex_rect.y0 / 4.0f );
	v2 xy1;
	v2 uv0( tex_rect.s / 32.0f, tex_rect.t / 32.0f );
	v2 uv1;

	//
	// In Fill/Copy mode the coordinates are inclusive (i.e. add 1.0f to the w/h)
	//
	switch ( gRDPOtherMode.cycle_type )
	{
		case CYCLE_COPY:
			d.x *= 0.25f;	// In copy mode 4 pixels are copied at once.
		case CYCLE_FILL:
			xy1.x = (tex_rect.x1 + 4) * 0.25f;
			xy1.y = (tex_rect.y1 + 4) * 0.25f;
			break;
		default:
			xy1.x = tex_rect.x1 * 0.25f;
			xy1.y = tex_rect.y1 * 0.25f;
			break;
	}

	uv1.x = uv0.x + d.x * ( xy1.y - xy0.y );		// Flip - use y
	uv1.y = uv0.y + d.y * ( xy1.x - xy0.x );		// Flip - use x

	DL_PF("    Screen(%.1f,%.1f) -> (%.1f,%.1f) Tile:%d ", xy0.x, xy0.y, xy1.x, xy1.y, tex_rect.tile_idx);
	DL_PF("    FLIPTex:(%#5.3f,%#5.3f) -> (%#5.3f,%#5.3f) (DSDX:%#5f DTDY:%#5f)", uv0.x, uv0.y, uv1.x, uv1.y, d.x, d.y);
	
	PSPRenderer::Get()->TexRectFlip( tex_rect.tile_idx, xy0, xy1, uv0, uv1 );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_FillRect( MicroCodeCommand command )
{ 
	// Note, in some modes, the right/bottom lines aren't drawn
	DL_PF("    (%d,%d) (%d,%d)", command.fillrect.x0, command.fillrect.y0, command.fillrect.x1, command.fillrect.y1);

	//Always clear Zbuffer if Depthbuffer is selected //Corn
	if (g_DI.Address == g_CI.Address)
	{
		CGraphicsContext::Get()->ClearZBuffer( 0 );
		DL_PF("    Clearing ZBuffer");
		return;
	}

	// Unless we support fb emulation, we can safetly ignore this fillrect
	if( bIsOffScreen )	return;

	// Removes unnecesary fillrects in Golden Eye and other games.
	//
	SCISSOR_RECT( command.fillrect.x0, command.fillrect.y0, 
				  command.fillrect.x1, command.fillrect.y1 );

	v2 xy0( command.fillrect.x0, command.fillrect.y0 );
	v2 xy1;

	//
	// In Fill/Copy mode the coordinates are inclusive (i.e. add 1.0f to the w/h)
	//
	switch ( gRDPOtherMode.cycle_type )
	{
		case CYCLE_COPY:
		case CYCLE_FILL:
			xy1.x = command.fillrect.x1 + 1;
			xy1.y = command.fillrect.y1 + 1;
			break;
		default:
			xy1.x = command.fillrect.x1;
			xy1.y = command.fillrect.y1;
			break;
	}

	// TODO - In 1/2cycle mode, skip bottom/right edges!?
	// This is done in PSPrenderer.

	// Clear the screen if large rectangle?
	// This seems to mess up with the Zelda game select screen
	// For some reason it draws a large rect over the entire
	// display, right at the end of the dlist. It sets the primitive
	// colour just before, so maybe I'm missing something??

	// TODO - Check colour image format to work out how this should be decoded!
	c32		colour;

	if ( g_CI.Size == G_IM_SIZ_16b )
	//if ( gRDPOtherMode.cycle_type != CYCLE_FILL )
	{
		DAEDALUS_ASSERT( gRDPOtherMode.cycle_type != CYCLE_FILL, "Fillrect : Check Me");

		PixelFormats::N64::Pf5551	c( (u16)gFillColor );

		colour = PixelFormats::convertPixelFormat< c32, PixelFormats::N64::Pf5551 >( c );

		//printf( "FillRect: %08x, %04x\n", colour.GetColour(), c.Bits );
		DL_PF("    Filling Rectangle");
		PSPRenderer::Get()->FillRect( xy0, xy1, colour.GetColour() );

	}
	else
	{
		// Used by Superman 64's sky.. 
		// There seems to be a bug with gFillColor? since the sky color is incorrect and dims when pause menu is showed up
		colour = c32( gFillColor );
		PSPRenderer::Get()->FillRect( xy0, xy1, colour.GetColour() );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetZImg( MicroCodeCommand command )
{
	DL_PF("    Image: 0x%08x", RDPSegAddr(command.inst.cmd1));

	g_DI.Address = RDPSegAddr(command.inst.cmd1);
}

//*****************************************************************************
//
//*****************************************************************************
//#define STORE_CI	{g_CI.Address = newaddr;g_CI.Format = format;g_CI.Size = size;g_CI.Width = width;g_CI.Bpl=Bpl;}

void DLParser_SetCImg( MicroCodeCommand command )
{
	u32 format = command.img.fmt;
	u32 size   = command.img.siz;
	u32 width  = command.img.width + 1;
	u32 newaddr	= RDPSegAddr(command.img.addr);
	//u32 bpl		= width << size >> 1;	// Do we need to handle?

	DL_PF("    Image: 0x%08x", RDPSegAddr(command.inst.cmd1));
	DL_PF("    Fmt: %s Size: %s Width: %d", gFormatNames[ format ], gSizeNames[ size ], width);

	// Not sure if this really necesary.
	//
	/*
	if( g_CI.Address == newaddr && g_CI.Format == format && g_CI.Size == size && g_CI.Width == width )
	{
		DL_PF("    Set CIMG to the same address, no change, skipped");
		//DBGConsole_Msg(0, "SetCImg: Addr=0x%08X, Fmt:%s-%sb, Width=%d\n", g_CI.Address, gFormatNames[ format ], gSizeNames[ size ], width);
		return;
	}*/

	//g_CI.Bpl = bpl;
	g_CI.Address = newaddr;
	g_CI.Format = format;
	g_CI.Size = size;
	g_CI.Width = width;

	// Used to remove offscreen, it removes the black box in the right side of the screen too :)
	// This will break FB, maybe add an option for this when FB is implemented?
	// Borrowed from Rice Video
	// Do not check texture size, it breaks Superman and Doom64..
	//
	bIsOffScreen = ( /*g_CI.Size != G_IM_SIZ_16b ||*/ g_CI.Format != G_IM_FMT_RGBA || g_CI.Width < 200 );
	//bIsOffScreen = false;
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetCombine( MicroCodeCommand command )
{
	union
	{
		u64		mux;
		u32		mpart[2];
	}un;

	un.mpart[0] = command.inst.cmd1;
	un.mpart[1] = command.inst.cmd0 & 0x00FFFFFF;

	PSPRenderer::Get()->SetMux( un.mux );
	
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		DLParser_DumpMux( un.mux );
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetFillColor( MicroCodeCommand command )
{
	gFillColor = command.inst.cmd1;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	PixelFormats::N64::Pf5551	n64col( (u16)gFillColor );
	DL_PF( "    Color5551=0x%04x", n64col.Bits );
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetFogColor( MicroCodeCommand command )
{
	DL_PF("    RGBA: %d %d %d %d", command.color.r, command.color.g, command.color.b, command.color.a);

	c32	fog_colour( command.color.r, command.color.g, command.color.b, command.color.a );

	PSPRenderer::Get()->SetFogColour( fog_colour );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetBlendColor( MicroCodeCommand command )
{
	DL_PF("    RGBA: %d %d %d %d", command.color.r, command.color.g, command.color.b, command.color.a);

	PSPRenderer::Get()->SetAlphaRef( command.color.a );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetPrimColor( MicroCodeCommand command )
{
	DL_PF("    M:%d L:%d RGBA: %d %d %d %d", command.color.prim_min_level, command.color.prim_level, command.color.r, command.color.g, command.color.b, command.color.a);

	c32	prim_colour( command.color.r, command.color.g, command.color.b, command.color.a );

	PSPRenderer::Get()->SetPrimitiveColour( prim_colour );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetEnvColor( MicroCodeCommand command )
{
	DL_PF("    RGBA: %d %d %d %d", command.color.r, command.color.g, command.color.b, command.color.a);

	c32	env_colour( command.color.r, command.color.g,command.color.b, command.color.a );

	PSPRenderer::Get()->SetEnvColour( env_colour );
}

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
u32 DLParser_GetInstructionCountLimit()
{
	return gInstructionCountLimit;
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
//RSP TRI commands..
//In HLE emulation you NEVER see this commands !
//*****************************************************************************
void DLParser_TriRSP( MicroCodeCommand command ){ DL_PF("RSP Tri: (Ignored)"); }

//*************************************************************************************
// 
//*************************************************************************************
#if 0 //1->unrolled, 0->looped //Corn
void MatrixFromN64FixedPoint( Matrix4x4 & mat, u32 address )
{
	struct N64Fmat
	{
		s16	mh01;
		s16	mh00;
		s16	mh03;
		s16	mh02;

		s16	mh11;
		s16	mh10;
		s16	mh13;
		s16	mh12;

		s16	mh21;
		s16	mh20;
		s16	mh23;
		s16	mh22;

		s16	mh31;
		s16	mh30;
		s16	mh33;
		s16	mh32;

		u16	ml01;
		u16	ml00;
		u16	ml03;
		u16	ml02;

		u16	ml11;
		u16	ml10;
		u16	ml13;
		u16	ml12;

		u16	ml21;
		u16	ml20;
		u16	ml23;
		u16	ml22;

		u16	ml31;
		u16	ml30;
		u16	ml33;
		u16	ml32;
	};

	const u8 * base( g_pu8RamBase );
	const N64Fmat * Imat = (N64Fmat *)(base + address);
	const f32 fRecip = 1.0f / 65536.0f;

	mat.m[0][0] = (f32)((Imat->mh00 << 16) | (Imat->ml00)) * fRecip;
	mat.m[0][1] = (f32)((Imat->mh01 << 16) | (Imat->ml01)) * fRecip;
	mat.m[0][2] = (f32)((Imat->mh02 << 16) | (Imat->ml02)) * fRecip;
	mat.m[0][3] = (f32)((Imat->mh03 << 16) | (Imat->ml03)) * fRecip;

	mat.m[1][0] = (f32)((Imat->mh10 << 16) | (Imat->ml10)) * fRecip;
	mat.m[1][1] = (f32)((Imat->mh11 << 16) | (Imat->ml11)) * fRecip;
	mat.m[1][2] = (f32)((Imat->mh12 << 16) | (Imat->ml12)) * fRecip;
	mat.m[1][3] = (f32)((Imat->mh13 << 16) | (Imat->ml13)) * fRecip;

	mat.m[2][0] = (f32)((Imat->mh20 << 16) | (Imat->ml20)) * fRecip;
	mat.m[2][1] = (f32)((Imat->mh21 << 16) | (Imat->ml21)) * fRecip;
	mat.m[2][2] = (f32)((Imat->mh22 << 16) | (Imat->ml22)) * fRecip;
	mat.m[2][3] = (f32)((Imat->mh23 << 16) | (Imat->ml23)) * fRecip;

	mat.m[3][0] = (f32)((Imat->mh30 << 16) | (Imat->ml30)) * fRecip;
	mat.m[3][1] = (f32)((Imat->mh31 << 16) | (Imat->ml31)) * fRecip;
	mat.m[3][2] = (f32)((Imat->mh32 << 16) | (Imat->ml32)) * fRecip;
	mat.m[3][3] = (f32)((Imat->mh33 << 16) | (Imat->ml33)) * fRecip;
}
#else
void MatrixFromN64FixedPoint( Matrix4x4 & mat, u32 address )
{
	const f32	fRecip = 1.0f / 65536.0f;
	const u8 *	base( g_pu8RamBase );
	s16 hi;
	u16 lo;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (address + 64 > MAX_RAM_ADDRESS)
	{
		DBGConsole_Msg(0, "Mtx: Address invalid (0x%08x)", address);
		return;
	}
#endif

	for (u32 i = 0; i < 4; i++)
	{
		hi = *(s16 *)(base + address+(i<<3)+(((0)     )^0x2));
		lo = *(u16 *)(base + address+(i<<3)+(((0) + 32)^0x2));
		mat.m[i][0] = ((hi<<16) | (lo)) * fRecip;

		hi = *(s16 *)(base + address+(i<<3)+(((2)     )^0x2));
		lo = *(u16 *)(base + address+(i<<3)+(((2) + 32)^0x2));
		mat.m[i][1] = ((hi<<16) | (lo)) * fRecip;

		hi = *(s16 *)(base + address+(i<<3)+(((4)     )^0x2));
		lo = *(u16 *)(base + address+(i<<3)+(((4) + 32)^0x2));
		mat.m[i][2] = ((hi<<16) | (lo)) * fRecip;

		hi = *(s16 *)(base + address+(i<<3)+(((6)     )^0x2));
		lo = *(u16 *)(base + address+(i<<3)+(((6) + 32)^0x2));
		mat.m[i][3] = ((hi<<16) | (lo)) * fRecip;
	}
}
#endif
//*************************************************************************************
//
//*************************************************************************************
// Rayman 2, Donald Duck, Tarzan, all wrestling games use this
//
static void RDP_Force_Matrix(u32 address)
{

	Matrix4x4 mat;
	MatrixFromN64FixedPoint(mat,address);

#if 1	//1->Proper, 0->Hacky way :)
	PSPRenderer::Get()->ForceMatrix(mat);
#else
	//WWF games dont like proper way need to figure out why...
	PSPRenderer::Get()->SetProjection(mat, true, true);
#endif
}