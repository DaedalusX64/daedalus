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


#include "Base/Types.h"


#include "Interface/ConfigOptions.h"
#include "Core/CPU.h"
#include "Core/Memory.h"
#include "Core/ROM.h"
#include "Debug/DBGConsole.h"
#include "Debug/Dump.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/NativePixelFormat.h"
#include "HLEGraphics/ConvertFormats.h"			// YUVtoRGBA
#include "HLEGraphics/DLDebug.h"
#include "HLEGraphics/DLParser.h"
#include "HLEGraphics/BaseRenderer.h"
#include "HLEGraphics/Microcode.h"
#include "HLEGraphics/N64PixelFormat.h"
#include "HLEGraphics/TextureCache.h"
#include "HLEGraphics/RDP.h"
#include "HLEGraphics/RDPStateManager.h"
#include "Utility/MathUtil.h"
#include "Ultra/ultra_gbi.h"
#include "Ultra/ultra_rcp.h"
#include "Ultra/ultra_sptask.h"
#include "HLEGraphics/GraphicsPlugin.h"
#include "Utility/BatchTest.h"
#include "uCodes/UcodeDefs.h"
#include "uCodes/Ucode.h"

#include "Utility/Profiler.h"



#ifdef DAEDALUS_DEBUG_DISPLAYLIST
#define DL_UNIMPLEMENTED_ERROR( msg )			\
{												\
	static bool shown = false;					\
	if (!shown )								\
	{											\
		DL_PF( "~*Not Implemented %s", msg );	\
		DAEDALUS_DL_ERROR( "%s: %08x %08x", (msg), command.inst.cmd0, command.inst.cmd1 );				\
		shown = true;							\
	}											\
}
#else
#define DL_UNIMPLEMENTED_ERROR( msg )
#endif

#define MAX_DL_STACK_SIZE 32

#define RDPSegAddr(seg)	( (gSegments[(seg >> 24) & 0x0F] + (seg & (MAX_RAM_ADDRESS-1))) & (MAX_RAM_ADDRESS-1))

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                     GFX State                        //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

struct N64Viewport
{
    s16 scale_y, scale_x, scale_w, scale_z;
	s16 trans_y, trans_x, trans_w, trans_z;
};

DAEDALUS_STATIC_ASSERT( sizeof(N64Viewport) == 16 );

struct N64mat
{
	struct _s16
	{
		s16 y, x, w, z;
	};

	struct _u16
	{
		u16 y, x, w, z;
	};

	_s16 h[4];
	_u16 l[4];
};

DAEDALUS_STATIC_ASSERT( sizeof(N64mat) == 64 );

struct N64Light
{
	u8 ca, b, g, r;					// Colour and ca (ca is different for conker)
	u8 la, b2, g2, r2;
	union
	{
		struct
		{
			s8 pad0, dir_z, dir_y, dir_x;	// Direction
			u8 pad1, qa, pad2, nonzero;
		};
		struct
		{
			s16 y1, x1, w1, z1;		// Position, GBI2 ex Majora's Mask
		};
	};
	s32 pad4, pad5, pad6, pad7;		// Padding..
	s16 y, x, w, z; 				// Position, Conker
};

DAEDALUS_STATIC_ASSERT( sizeof(N64Light) == 40 );

struct RDP_Scissor
{
	u32 left, top, right, bottom;
};

// The display list PC stack. Before this was an array of 10
// items, but this way we can nest as deeply as necessary.
struct DList
{
	u32 address[MAX_DL_STACK_SIZE];
	s32 limit;
};

//*****************************************************************************
//
//*****************************************************************************
void RDP_MoveMemViewport(u32 address);
void MatrixFromN64FixedPoint( Matrix4x4 & mat, u32 address );
void DLParser_InitMicrocode( u32 code_base, u32 code_size, u32 data_base, u32 data_size );

enum LightSource
{
	POINT_LIGHT_NONE = 0,	// No Point Light
	POINT_LIGHT_MM,			// Majora's Mask Point Light
	POINT_LIGHT_CBFD,		// Conker's Point Light
	POINT_LIGHT_ACCLAIM		// Acclaim's Point Light (unsupported ATM)
};
template< LightSource Source, u32 LightSize > 
void RDP_MoveMemLight(u32 address, u32 light_idx);


// Used to keep track of when we're processing the first display list
static bool gFirstCall = true;

static u32				gSegments[16];
static RDP_Scissor		scissors;
static RDP_GeometryMode gGeometryMode;
static DList			gDlistStack;
static s32				gDlistStackPointer = -1;
static u32				gRDPHalf1 = 0;

       SImageDescriptor g_TI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };
static SImageDescriptor g_CI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };
static SImageDescriptor g_DI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };

const MicroCodeInstruction *gUcodeFunc = gNormalInstruction[ GBI_0 ];
static const char ** gUcodeName = gNormalInstructionName[ GBI_0 ];

bool gFrameskipActive = false;

// Define this to validate an address before referencing an N64 ram location
#ifndef DAEDALUS_PSP
#define VALIDATE_ADDRESS_SEG
#endif

// Always call this function before referencing an N64 ram location
// In most cases we are already in range, see RDPSegAddr. No need to call this when size its 0
static inline bool IsAddressValid(u32 address, u32 size, const char * name)
{
#ifdef VALIDATE_ADDRESS_SEG	
	if ((address + size) > MAX_RAM_ADDRESS) 
	{
		DBGConsole_Msg(0,"%s: Address is out of range (0x%08x)", name, address);
		return false;
	}
#endif
	return true;
}

// Same as above but adds coverage for n (vert num) and checks the vertex index
// We call this earlier on the pipeline to prevent passing an invalid vertex info to the Dlist debugger!
static inline bool IsVertexInfoValid(u32 address, u32 size, u32 v0, u32 n)
{
#ifdef VALIDATE_ADDRESS_SEG	
	if ((address + (n * size)) > MAX_RAM_ADDRESS) 
	{
		DBGConsole_Msg(0,"VertexInfo: Address is out of range (0x%08x)", address);
		return false;
	}
#endif
	if ((n + v0) > kMaxN64Vertices) 
	{
		DAEDALUS_ERROR("VertexInfo: Vertex index is out of bounds (%d)", (n + v0));
		return false;
	}
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
inline void FinishRDPJob()
{
	Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_DP);
	gCPUState.AddJob(CPU_CHECK_INTERRUPTS);
}

//*****************************************************************************
// Reads the next command from the display list, updates the PC.
//*****************************************************************************
inline bool	DLParser_FetchNextCommand( MicroCodeCommand * p_command )
{
	// Current PC is the last value on the stack
	u32 & pc( gDlistStack.address[gDlistStackPointer] );
	if( !IsAddressValid(pc, 8, "FetchNextCommand") )
		return false;

	*p_command = *(MicroCodeCommand*)(g_pu8RamBase + pc);
	pc += 8;

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
inline void DLParser_PopDL()
{
	DL_PF("    Returning from DisplayList: level=%d", gDlistStackPointer+1);
	DL_PF("    ############################################");
	DL_PF("    /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\");
	DL_PF(" ");

	gDlistStackPointer--;
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//////////////////////////////////////////////////////////
//                      Debug vars                      //
//////////////////////////////////////////////////////////
void DLParser_DumpVtxInfo(u32 address, u32 v0_idx, u32 num_verts);

u32			gNumDListsCulled;
u32			gNumVertices;
u32			gNumRectsClipped;
u32			gNumInstructionsExecuted = 0;
#endif

//*****************************************************************************
//
//*****************************************************************************
u32 gRDPFrame = 0;
u32 gAuxAddr = 0;

extern u32 uViWidth;
extern u32 uViHeight;
//*****************************************************************************
// Include ucode header files
//*****************************************************************************
#include "uCodes/Ucode_GBI0.h"
#include "uCodes/Ucode_GBI1.h"
#include "uCodes/Ucode_GBI2.h"
#include "uCodes/Ucode_DKR.h"
#include "uCodes/Ucode_FB.h"
#include "uCodes/Ucode_GE.h"
#include "uCodes/Ucode_PD.h"
#include "uCodes/Ucode_Conker.h"
#include "uCodes/Ucode_LL.h"
#include "uCodes/Ucode_Beta.h"
#include "uCodes/Ucode_Sprite2D.h"
#include "uCodes/Ucode_S2DEX.h"

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                      Strings                         //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

static const char * const gFormatNames[8] = {"RGBA", "YUV", "CI", "IA", "I", "?1", "?2", "?3"};

static const char * const gSizeNames[4]   = {"4bpp", "8bpp", "16bpp", "32bpp"};
static const char * const gOnOffNames[2]  = {"Off", "On"};

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*****************************************************************************
//
//*****************************************************************************
void DLParser_DumpVtxInfo(u32 address, u32 v0_idx, u32 num_verts)
{
	if (DLDebug_IsActive())
	{
		s8 *pcSrc = (s8 *)(g_pu8RamBase + address);
		s16 *psSrc = (s16 *)(g_pu8RamBase + address);

		for ( u32 idx = v0_idx; idx < v0_idx + num_verts; idx++ )
		{
			f32 x = f32(psSrc[0^0x1]);
			f32 y = f32(psSrc[1^0x1]);
			f32 z = f32(psSrc[2^0x1]);

			u16 wFlags = u16(gRenderer->GetVtxFlags( idx )); //(u16)psSrc[3^0x1];

			u8 a = pcSrc[12^0x3];
			u8 b = pcSrc[13^0x3];
			u8 c = pcSrc[14^0x3];
			u8 d = pcSrc[15^0x3];

			s16 nTU = psSrc[4^0x1];
			s16 nTV = psSrc[5^0x1];

			f32 tu = f32(nTU) * (1.0f / 32.0f);
			f32 tv = f32(nTV) * (1.0f / 32.0f);

			const v4 & t = gRenderer->GetTransformedVtxPos( idx );
			const v4 & p = gRenderer->GetProjectedVtxPos( idx );

			psSrc += 8;			// Increase by 16 bytes
			pcSrc += 16;

			DL_PF("    #%02d Flags: 0x%04x Pos:{% 0.1f,% 0.1f,% 0.1f} Tex:{% 7.2f,% 7.2f} Extra: %02x %02x %02x %02x Tran:{% 0.3f,% 0.3f,% 0.3f,% 0.3f} Proj:{% 6f,% 6f,% 6f,% 6f}",
				idx, wFlags, x, y, z, tu, tv, a, b, c, d, t.x, t.y, t.z, t.w, p.x/p.w, p.y/p.w, p.z/p.w, p.w);
		}
	}
}
#endif

//*****************************************************************************
//
//*****************************************************************************
bool DLParser_Initialise()
{
	gFirstCall = true;
	gRDPFrame = 0;
	gAuxAddr = 0;
	gRDPHalf1 = 0;

	// Reset scissor to default
	scissors.top = 0;
	scissors.left = 0;
	scissors.right = 320;
	scissors.bottom = 240;

	// Clear ucode cache
	GBIMicrocode_Reset();

	// Init with Fast 3D ucode incase of a failure to start the ucode detection
	gUcodeFunc = gNormalInstruction[ GBI_0 ];
	gUcodeName = gNormalInstructionName[ GBI_0 ];

	//Clear pointers in TMEM block //Corn
	#ifdef DAEDALUS_ACCURATE_TMEM
	memset(gTlutLoadAddresses, 0, sizeof(gTlutLoadAddresses));
	#endif
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_Finalise()
{
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_InitMicrocode( u32 code_base, u32 code_size, u32 data_base, u32 data_size )
{
	if( !IsAddressValid(code_base, code_size, "InitMicrocode: ucode code") 
		|| !IsAddressValid(data_base, data_size, "InitMicrocode: ucode data")  )
	{
		return;
	}
	const UcodeInfo& ucode_info( GBIMicrocode_DetectVersion(code_base, code_size, data_base, data_size) );
	gUcodeFunc = ucode_info.func;
	gUcodeName = ucode_info.name;
}

//*****************************************************************************
//
//*****************************************************************************
#ifdef DAEDALUS_ENABLE_PROFILING
SProfileItemHandle * gpProfileItemHandles[ 256 ];

#define PROFILE_DL_CMD( cmd )								\
	if(gpProfileItemHandles[ (cmd) ] == NULL)				\
	{														\
		gpProfileItemHandles[ (cmd) ] = new SProfileItemHandle( CProfiler::Get()->AddItem( gUcodeName[ cmd ] ));		\
	}														\
	CAutoProfile		_auto_profile( *gpProfileItemHandles[ (cmd) ] )

#else

#define PROFILE_DL_CMD( cmd )		do { } while(0)

#endif

//*****************************************************************************
//	Process the entire display list in one go
//*****************************************************************************
static u32 DLParser_ProcessDList(u32 instruction_limit)
{
	MicroCodeCommand command;

	u32 current_instruction_count = 0;

	while(gDlistStackPointer >= 0)
	{
		if (!DLParser_FetchNextCommand( &command ))
			break;

		DL_BEGIN_INSTR(current_instruction_count, command.inst.cmd0, command.inst.cmd1, gDlistStackPointer, gUcodeName[command.inst.cmd]);

		PROFILE_DL_CMD( command.inst.cmd );

		gUcodeFunc[ command.inst.cmd ]( command );

		DL_END_INSTR();

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		// Note: make sure have frame skip disabled for the dlist debugger to work
		if( instruction_limit != kUnlimitedInstructionCount )
		{
			if( current_instruction_count >= instruction_limit )
			{
				return current_instruction_count;
			}
		}
		current_instruction_count++;
#endif

		// Check limit
		if (gDlistStack.limit >= 0)
		{
			if (--gDlistStack.limit < 0)
			{
				DL_PF("**EndDLInMem");
				gDlistStackPointer--;
				// limit is already reset to default -1 at this point
				//gDlistStack.limit = -1;
			}
		}
	}

	return current_instruction_count;
}

//*****************************************************************************
//
//*****************************************************************************
u32 DLParser_Process(u32 instruction_limit, DLDebugOutput * debug_output)
{
	DAEDALUS_PROFILE( "DLParser_Process" );

	if ( !CGraphicsContext::Get()->IsInitialised() || !gRenderer )
	{
		return 0;
	}

	// Shut down the debug console when we start rendering
	// TODO: Clear the front/backbuffer the first time this function is called
	// to remove any stuff lingering on the screen.
	if(gFirstCall)
	{
		CGraphicsContext::Get()->ClearAllSurfaces();

		gFirstCall = false;
	}

	// Update Screen only when something is drawn, otherwise several games ex Army Men will flash or shake.
	if( g_ROM.GameHacks != CHAMELEON_TWIST_2 ) gGraphicsPlugin->UpdateScreen();

	OSTask * pTask = (OSTask *)(g_pu8SpMemBase + 0x0FC0);
	u32 code_base = (u32)pTask->t.ucode & 0x1fffffff;
	//u32 code_size = pTask->t.ucode_size; // Conker sets this to 0..
	u32 code_size = 0x1000;
	u32 data_base = (u32)pTask->t.ucode_data & 0x1fffffff;
	u32 data_size = pTask->t.ucode_data_size;
	u32 stack_size = pTask->t.dram_stack_size >> 6;
	
	DLParser_InitMicrocode( code_base, code_size, data_base, data_size );

	//
	// Not sure what to init this with. We should probably read it from the dmem
	//
	gRDPOtherMode.L = 0x00500001;
	gRDPOtherMode.H = 0;

	gRDPFrame++;

	CTextureCache::Get()->PurgeOldTextures();

	// Initialise stack
	gDlistStackPointer=0;
	gDlistStack.address[0] = (u32)pTask->t.data_ptr;
	gDlistStack.limit = -1;

	gRDPStateManager.Reset();

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	gNumDListsCulled = 0;
	gNumVertices = 0;
	gNumRectsClipped = 0;
	if (debug_output)
		DLDebug_SetOutput(debug_output);
	DLDebug_DumpTaskInfo( pTask );
#endif

	DL_PF("DP: Firing up RDP!");

	u32 count = 0;

	if(!gFrameskipActive)
	{
		gRenderer->SetVIScales();
		gRenderer->ResetMatrices(stack_size);
		gRenderer->Reset();
		gRenderer->BeginScene();
		count = DLParser_ProcessDList(instruction_limit);
		gRenderer->EndScene();
	}
	else
	{
		FinishRDPJob();
	}

	// Hack for Chameleon Twist 2, only works if screen is update at last
	if( g_ROM.GameHacks == CHAMELEON_TWIST_2 ) 
		gGraphicsPlugin->UpdateScreen();

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DLDebug_SetOutput(NULL);

	// NB: only update gNumInstructionsExecuted when we rendered something.
	// I'd really like to get rid of gNumInstructionsExecuted.
	if (!gFrameskipActive)
		gNumInstructionsExecuted = count;
#endif

#ifdef DAEDALUS_BATCH_TEST_ENABLED
	CBatchTestEventHandler * handler( BatchTest_GetHandler() );
	if( handler )
		handler->OnDisplayListComplete();
#endif

	return count;
}

//*****************************************************************************
//
//*****************************************************************************
void MatrixFromN64FixedPoint( Matrix4x4 & mat, u32 address )
{
	if( !IsAddressValid(address, 64, "MatrixFromN64FixedPoint") )
		return;

	const f32 fRecip = 1.0f / 65536.0f;
	const N64mat *Imat = (N64mat *)( g_pu8RamBase + address );

	for (u32 i = 0; i < 4; i++)
	{
		mat.m[i][0] = ((Imat->h[i].x << 16) | Imat->l[i].x) * fRecip;
		mat.m[i][1] = ((Imat->h[i].y << 16) | Imat->l[i].y) * fRecip;
		mat.m[i][2] = ((Imat->h[i].z << 16) | Imat->l[i].z) * fRecip;
		mat.m[i][3] = ((Imat->h[i].w << 16) | Imat->l[i].w) * fRecip;
	}
}

//*****************************************************************************
//
//*****************************************************************************
template< LightSource Source, u32 LightSize > 
void RDP_MoveMemLight(u32 address, u32 light_idx)
{
	if( Source == POINT_LIGHT_ACCLAIM )
	{
		DAEDALUS_ERROR("ACCLAIM Lightning not supported");
		return;

	}

	if( light_idx >= LightSize )
	{
		DBGConsole_Msg(0, "Warning: invalid light # = %d, Max light # = %d", light_idx, LightSize);
	}
	else
	{
		if( !IsAddressValid(address, 40, "RDP_MoveMemLight") )
			return;

		const N64Light *light = (const N64Light*)(g_pu8RamBase + address);

		u8 r = light->r;
		u8 g = light->g;
		u8 b = light->b;

		s8 dir_x = light->dir_x;
		s8 dir_y = light->dir_y;
		s8 dir_z = light->dir_z;

		DL_PF("    Light[%d] RGB[%d, %d, %d] x[%d] y[%d] z[%d]", light_idx, r, g, b, dir_x, dir_y, dir_z);

		//Color
		gRenderer->SetLightCol( light_idx, r, g, b );

		//Direction
		gRenderer->SetLightDirection( light_idx, dir_x, dir_y, dir_z );

		//Position and Ambient light for Majora's Mask
		if( Source == POINT_LIGHT_MM )
		{ 
			gRenderer->SetLightPosition(light_idx, light->x1, light->y1, light->z1, 1.0f);
			gRenderer->SetLightEx(light_idx, light->ca, light->la, light->qa);
		}

		//Position and Ambient light for Conker
		if( Source == POINT_LIGHT_CBFD )
		{ 
			gRenderer->SetLightPosition( light_idx, light->x, light->y, light->z , light->w);
			gRenderer->SetLightCBFD( light_idx, light->nonzero);
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
	if( !IsAddressValid(address, 16, "RDP_MoveMemViewport") )
		return;

	// address is offset into RD_RAM of 8 x 16bits of data...
	N64Viewport *vp = (N64Viewport*)(g_pu8RamBase + address);

	// With D3D we had to ensure that the vp coords are positive, so
	// we truncated them to 0. This happens a lot, as things
	// seem to specify the scale as the screen w/2 h/2

	v2 vec_scale( vp->scale_x * 0.25f, vp->scale_y * 0.25f );
	v2 vec_trans( vp->trans_x * 0.25f, vp->trans_y * 0.25f );

	gRenderer->SetN64Viewport( vec_scale, vec_trans );

	DL_PF("    Scale: %d %d", vp->scale_x, vp->scale_y);
	DL_PF("    Trans: %d %d", vp->trans_x, vp->trans_y);
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
void DLParser_SetKeyGB( MicroCodeCommand command [[maybe_unused]] )
{
	DL_PF( "    SetKeyGB (Ignored)" );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetKeyR( MicroCodeCommand command [[maybe_unused]] )
{
	DL_PF( "    SetKeyR (Ignored)" );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetConvert( MicroCodeCommand command [[maybe_unused]] )
{
	DL_PF( "    SetConvert (Ignored)" );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetPrimDepth( MicroCodeCommand command )
{
	DL_PF("    SetPrimDepth z[0x%04x] dz[0x%04x]",
		command.primdepth.z, command.primdepth.dz);

	gRenderer->SetPrimitiveDepth( command.primdepth.z );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_RDPSetOtherMode( MicroCodeCommand command )
{
	DL_PF( "    RDPSetOtherMode: 0x%08x 0x%08x", command.inst.cmd0, command.inst.cmd1 );

	gRDPOtherMode.H = command.inst.cmd0;
	gRDPOtherMode.L = command.inst.cmd1;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DLDebug_DumpRDPOtherMode(gRDPOtherMode);
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_RDPLoadSync( MicroCodeCommand command  [[maybe_unused]] )	{ /*DL_PF("    LoadSync: (Ignored)");*/ }
void DLParser_RDPPipeSync( MicroCodeCommand command  [[maybe_unused]])	{ /*DL_PF("    PipeSync: (Ignored)");*/ }
void DLParser_RDPTileSync( MicroCodeCommand command  [[maybe_unused]])	{ /*DL_PF("    TileSync: (Ignored)");*/ }

//*****************************************************************************
//
//*****************************************************************************
void DLParser_RDPFullSync( MicroCodeCommand command [[maybe_unused]] )
{
	DL_PF("    FullSync: (Generating Interrupt)");
	FinishRDPJob();
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetScissor( MicroCodeCommand command )
{
	// The coords are all in 10:2 fixed point
	// Set up scissoring zone, we'll use it to scissor other stuff ex Texrect
	//
	scissors.left    = command.scissor.x0>>2;
	scissors.top     = command.scissor.y0>>2;
	scissors.right   = command.scissor.x1>>2;
	scissors.bottom  = command.scissor.y1>>2;

	// Hack to correct Super Bowling's right and left screens
	if ( g_ROM.GameHacks == SUPER_BOWLING && g_CI.Address%0x100 != 0 )
	{
		scissors.left += 160;
		scissors.right += 160;
		v2 vec_trans( 240, 120 );
		v2 vec_scale( 80, 120 );
		gRenderer->SetN64Viewport( vec_scale, vec_trans );
	}

	DL_PF("    x0=%d y0=%d x1=%d y1=%d mode=%d", scissors.left, scissors.top, scissors.right, scissors.bottom, command.scissor.mode);

	// Set the cliprect now...
	if ( scissors.left < scissors.right && scissors.top < scissors.bottom )
	{
		gRenderer->SetScissor( scissors.left, scissors.top, scissors.right, scissors.bottom );
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

	DL_PF( "    Tile[%d] Format[%s/%s] Line[%d] TMEM[0x%03x] Palette[%d]", tile.tile_idx, gFormatNames[tile.format], gSizeNames[tile.size], tile.line, tile.tmem, tile.palette);
	DL_PF( "      S: Clamp[%s] Mirror[%s] Mask[0x%x] Shift[0x%x]", gOnOffNames[tile.clamp_s],gOnOffNames[tile.mirror_s], tile.mask_s, tile.shift_s );
	DL_PF( "      T: Clamp[%s] Mirror[%s] Mask[0x%x] Shift[0x%x]", gOnOffNames[tile.clamp_t],gOnOffNames[tile.mirror_t], tile.mask_t, tile.shift_t );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetTileSize( MicroCodeCommand command )
{
	RDP_TileSize tile;
	tile.cmd0 = command.inst.cmd0;
	tile.cmd1 = command.inst.cmd1;

	DL_PF("    Tile[%d] (%d,%d) -> (%d,%d) [%d x %d]",
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

	DL_PF("    TImg Adr[0x%08x] Format[%s/%s] Width[%d] Pitch[%d]",
		g_TI.Address, gFormatNames[g_TI.Format], gSizeNames[g_TI.Size], g_TI.Width, g_TI.GetPitch());
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_LoadBlock( MicroCodeCommand command )
{
	gRDPStateManager.LoadBlock( command.loadtile );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_LoadTile( MicroCodeCommand command )
{
	gRDPStateManager.LoadTile( command.loadtile );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_LoadTLut( MicroCodeCommand command )
{
	gRDPStateManager.LoadTlut( command.loadtile );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_TexRect( MicroCodeCommand command )
{
	MicroCodeCommand command2;
	MicroCodeCommand command3;

	if( !DLParser_FetchNextCommand( &command2 ) ||
		!DLParser_FetchNextCommand( &command3 ) )
		return;

	RDP_TexRect tex_rect;
	tex_rect.cmd0 = command.inst.cmd0;
	tex_rect.cmd1 = command.inst.cmd1;
	tex_rect.cmd2 = command2.inst.cmd1;
	tex_rect.cmd3 = command3.inst.cmd1;

	DAEDALUS_DL_ASSERT(gRDPOtherMode.cycle_type != CYCLE_COPY || tex_rect.dsdx == (4<<10), "Expecting dsdx of 4<<10 in copy mode, got %d", tex_rect.dsdx);

	// NB: In FILL and COPY mode, rectangles are scissored to the nearest four pixel boundary.
	// This isn't currently handled, but I don't know of any games that depend on it.

	//Keep integers for as long as possible //Corn

	// X for upper left corner should be less than X for lower right corner else skip rendering it, seems to happen in Rayman 2 and Star Soldier
	//if( tex_rect.x0 >= tex_rect.x1 )

	// Hack for Banjo Tooie shadow
	if (g_ROM.GameHacks == BANJO_TOOIE && gRDPOtherMode.L == 0x00504241)
	{
		return;
	}

	// Fixes black box in SSB when moving far way from the screen and offscreen in Conker
	if (g_DI.Address == g_CI.Address || g_CI.Format != G_IM_FMT_RGBA)
	{
		DL_PF("    Ignoring Texrect");
		return;
	}

	// Removes offscreen texrect, also fixes several glitches like in John Romero's Daikatana
	if( tex_rect.x0 >= (scissors.right<<2) ||
		tex_rect.y0 >= (scissors.bottom<<2) ||
		tex_rect.x1 <  (scissors.left<<2) ||
		tex_rect.y1 <  (scissors.top<<2) )
	{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		++gNumRectsClipped;
#endif
		return;
	};

	s16 rect_s0 = tex_rect.s;
	s16 rect_t0 = tex_rect.t;

	s32 rect_dsdx = tex_rect.dsdx;
	s32 rect_dtdy = tex_rect.dtdy;

	rect_s0 += (((u32)rect_dsdx >> 31) << 5);	//Fixes California Speed, if(rect_dsdx<0) rect_s0 += 32;
	rect_t0 += (((u32)rect_dtdy >> 31) << 5);

	// In Fill/Copy mode the coordinates are inclusive (i.e. add 1<<2 to the w/h)
	u32 cycle_mode = gRDPOtherMode.cycle_type;
	if ( cycle_mode >= CYCLE_COPY )
	{
		// In copy mode 4 pixels are copied at once.
		if ( cycle_mode == CYCLE_COPY )
			rect_dsdx = rect_dsdx >> 2;

		tex_rect.x1 += 4;
		tex_rect.y1 += 4;
	}

	s16 rect_s1 = rect_s0 + (rect_dsdx * ( tex_rect.x1 - tex_rect.x0 ) >> 7);	// 7 = (>>10)=1/1024, (>>2)=1/4 and (<<5)=32
	s16 rect_t1 = rect_t0 + (rect_dtdy * ( tex_rect.y1 - tex_rect.y0 ) >> 7);

	TexCoord st0( rect_s0, rect_t0 );
	TexCoord st1( rect_s1, rect_t1 );

	v2 xy0( tex_rect.x0 / 4.0f, tex_rect.y0 / 4.0f );
	v2 xy1( tex_rect.x1 / 4.0f, tex_rect.y1 / 4.0f );

	DL_PF("    Screen(%.1f,%.1f) -> (%.1f,%.1f) Tile[%d]", xy0.x, xy0.y, xy1.x, xy1.y, tex_rect.tile_idx);
	DL_PF("    Tex:(%#5.3f,%#5.3f) -> (%#5.3f,%#5.3f) (DSDX:%#5f DTDY:%#5f)", rect_s0/32.f, rect_t0/32.f, rect_s1/32.f, rect_t1/32.f, rect_dsdx/1024.f, rect_dtdy/1024.f);

	gRenderer->TexRect( tex_rect.tile_idx, xy0, xy1, st0, st1 );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_TexRectFlip( MicroCodeCommand command )
{
	MicroCodeCommand command2;
	MicroCodeCommand command3;

	if( !DLParser_FetchNextCommand( &command2 ) ||
		!DLParser_FetchNextCommand( &command3 ) )
		return;

	RDP_TexRect tex_rect;
	tex_rect.cmd0 = command.inst.cmd0;
	tex_rect.cmd1 = command.inst.cmd1;
	tex_rect.cmd2 = command2.inst.cmd1;
	tex_rect.cmd3 = command3.inst.cmd1;

	DAEDALUS_DL_ASSERT(gRDPOtherMode.cycle_type != CYCLE_COPY || tex_rect.dsdx == (4<<10), "Expecting dsdx of 4<<10 in copy mode, got %d", tex_rect.dsdx);

	//Keep integers for as long as possible //Corn

	s16 rect_s0 = tex_rect.s;
	s16 rect_t0 = tex_rect.t;

	s32 rect_dsdx = tex_rect.dsdx;
	s32 rect_dtdy = tex_rect.dtdy;

	rect_s0 += (((u32)rect_dsdx >> 31) << 5);	// For Wetrix
	rect_t0 += (((u32)rect_dtdy >> 31) << 5);

	// In Fill/Copy mode the coordinates are inclusive (i.e. add 1<<2 to the w/h)
	u32 cycle_mode = gRDPOtherMode.cycle_type;
	if ( cycle_mode >= CYCLE_COPY )
	{
		// In copy mode 4 pixels are copied at once.
		if ( cycle_mode == CYCLE_COPY )
			rect_dsdx = rect_dsdx >> 2;

		tex_rect.x1 += 4;
		tex_rect.y1 += 4;
	}

	s16 rect_s1 = rect_s0 + (rect_dsdx * ( tex_rect.y1 - tex_rect.y0 ) >> 7);	// Flip - use y
	s16 rect_t1 = rect_t0 + (rect_dtdy * ( tex_rect.x1 - tex_rect.x0 ) >> 7);	// Flip - use x

	TexCoord st0( rect_s0, rect_t0 );
	TexCoord st1( rect_s1, rect_t1 );

	v2 xy0( tex_rect.x0 / 4.0f, tex_rect.y0 / 4.0f );
	v2 xy1( tex_rect.x1 / 4.0f, tex_rect.y1 / 4.0f );

	DL_PF("    Screen(%.1f,%.1f) -> (%.1f,%.1f) Tile[%d]", xy0.x, xy0.y, xy1.x, xy1.y, tex_rect.tile_idx);
	DL_PF("    FlipTex:(%#5.3f,%#5.3f) -> (%#5.3f,%#5.3f) (DSDX:%#5f DTDY:%#5f)", rect_s0/32.f, rect_t0/32.f, rect_s1/32.f, rect_t1/32.f, rect_dsdx/1024.f, rect_dtdy/1024.f);

	gRenderer->TexRectFlip( tex_rect.tile_idx, xy0, xy1, st0, st1 );
}

//Clear framebuffer, thanks Gonetz! http://www.emutalk.net/threads/15818-How-to-implement-quot-emulate-clear-quot-Answer-and-Question
//This fixes the jumpy camera in DK64, also the sun and flames glare in Zelda
void Clear_N64DepthBuffer( MicroCodeCommand command )
{
	u32 x0 = command.fillrect.x0 + 1;
	u32 x1 = command.fillrect.x1 + 1;
	u32 y1 = command.fillrect.y1;
	u32 y0 = command.fillrect.y0;

	// Using s32 to force min/max to be done in a single op code for the PSP
	x0 = std::min(std::max(x0, scissors.left), scissors.right);
	x1 = std::min(std::max(x0, scissors.left), scissors.right);
	y0 = std::min(std::max(y0, scissors.top), scissors.bottom);
	y1 = std::min(std::max(y1, scissors.top), scissors.bottom);
	
	//x0 = Min<s32>(Max<s32>(x0, scissors.left), scissors.right);
	// x1 = Min<s32>(Max<s32>(x1, scissors.left), scissors.right);
	// y1 = Min<s32>(Max<s32>(y1, scissors.top), scissors.bottom);
	// y0 = Min<s32>(Max<s32>(y0, scissors.top), scissors.bottom);
	x0 >>= 1;
	x1 >>= 1;
	u32 zi_width_in_dwords = g_CI.Width >> 1;
	u32 fill_colour = gRenderer->GetFillColour();
	u32 * dst = (u32*)(g_pu8RamBase + g_CI.Address) + y0 * zi_width_in_dwords;

	for( u32 y = y0; y <y1; y++ )
	{
		for( u32 x = x0; x < x1; x++ )
		{
			dst[x] = fill_colour;
		}
		dst += zi_width_in_dwords;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_FillRect( MicroCodeCommand command )
{
	//Always clear Zbuffer if Depthbuffer is selected //Corn
	if (g_DI.Address == g_CI.Address)
	{
		CGraphicsContext::Get()->ClearZBuffer();

#ifdef DAEDALUS_PSP
		if(gClearDepthFrameBuffer)
#else
		if(true)	//This always enabled for PC, this should be optional once we have a GUI to disable it!
#endif
		{
			Clear_N64DepthBuffer(command);
		}
		DL_PF("    Clearing ZBuffer");
		return;
	}

	// Removes annoying rect that appears in Conker and fillrects that cover screen in banjo tooie
	if( g_CI.Format != G_IM_FMT_RGBA )
	{
		DL_PF("    Ignoring Fillrect ");
		return;
	}

	u32 x0 = command.fillrect.x0;
	u32 x1 = command.fillrect.x1;
	u32 y1 = command.fillrect.y1;
	u32 y0 = command.fillrect.y0;

	// Note, in some modes, the right/bottom lines aren't drawn
	// TODO - Check colour image format to work out how this should be decoded!
	// Should we init with Prim or Blend colour? Doesn't work well see Mk64 transition before a race
	c32	colour = c32(0);
	u32 cycle_mode = gRDPOtherMode.cycle_type;
	//
	// In Fill/Copy mode the coordinates are inclusive (i.e. add 1.0f to the w/h)
	//
	if ( cycle_mode >= CYCLE_COPY )
	{
		x1++;
		y1++;

		if ( cycle_mode == CYCLE_FILL )
		{
			u32 fill_colour = gRenderer->GetFillColour();
			if(g_CI.Size == G_IM_SIZ_16b)
			{
				const N64Pf5551	c( (u16)fill_colour );
				colour = ConvertPixelFormat< c32, N64Pf5551 >( c );
			}
			else
			{
				const N64Pf8888	c( (u32)fill_colour );
				colour = ConvertPixelFormat< c32, N64Pf8888 >( c );
			}

			// Clear the screen if its just a large rectangle
			if( (x1 - x0) == uViWidth && (y1 - y0) == uViHeight )
			{
				CGraphicsContext::Get()->ClearColBuffer( colour );
				DL_PF("    Clearing Colour Buffer");
				return;
			}
		}
	}
	DL_PF("    Filling Rectangle (%d,%d)->(%d,%d)", x0, y0, x1, y1);

	v2 xy0( (f32)x0, (f32)y0 );
	v2 xy1( (f32)x1, (f32)y1 );

	// TODO - In 1/2cycle mode, skip bottom/right edges!?
	// This is done in BaseRenderer.

	gRenderer->FillRect( xy0, xy1, colour.GetColour() );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetZImg( MicroCodeCommand command )
{
	g_DI.Address = RDPSegAddr(command.inst.cmd1);
	DL_PF("    ZImg Adr[0x%08x]", g_DI.Address);
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetCImg( MicroCodeCommand command )
{
	g_CI.Format = command.img.fmt;
	g_CI.Size   = command.img.siz;
	g_CI.Width  = command.img.width + 1;
	g_CI.Address = RDPSegAddr(command.img.addr);
	//g_CI.Bpl		= g_CI.Width << g_CI.Size >> 1;

	DL_PF("    CImg Adr[0x%08x] Format[%s] Size[%s] Width[%d]", g_CI.Address, gFormatNames[ g_CI.Format ], gSizeNames[ g_CI.Size ], g_CI.Width);
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetCombine( MicroCodeCommand command )
{
	//Swap the endian
	REG64 Mux;
	Mux._u32_0 = command.inst.cmd1;
	Mux._u32_1 = command.inst.arg0;

	gRenderer->SetMux( Mux._u64 );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (DLDebug_IsActive())
	{
		DLDebug_DumpMux( Mux._u64 );
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetFillColor( MicroCodeCommand command )
{
	u32 fill_colour = command.inst.cmd1;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	N64Pf5551	n64col( (u16)fill_colour );
	DL_PF( "    Color5551=0x%04x", n64col.Bits );
#endif

	gRenderer->SetFillColour( fill_colour );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetFogColor( MicroCodeCommand command )
{
	DL_PF("    RGBA: %d %d %d %d", command.color.r, command.color.g, command.color.b, command.color.a);

	//c32	fog_colour( command.color.r, command.color.g, command.color.b, command.color.a );
	c32	fog_colour( command.color.r, command.color.g, command.color.b, 0 );	//alpha is always 0

	gRenderer->SetFogColour( fog_colour );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetBlendColor( MicroCodeCommand command )
{
	DL_PF("    RGBA: %d %d %d %d", command.color.r, command.color.g, command.color.b, command.color.a);

	c32	blend_colour( command.color.r, command.color.g, command.color.b, command.color.a );

	gRenderer->SetBlendColour( blend_colour );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetPrimColor( MicroCodeCommand command )
{
	DL_PF("    M:%d L:%d RGBA: %d %d %d %d", command.color.prim_min_level, command.color.prim_level, command.color.r, command.color.g, command.color.b, command.color.a);

	c32	prim_colour( command.color.r, command.color.g, command.color.b, command.color.a );

	gRenderer->SetPrimitiveLODFraction(command.color.prim_level / 256.f);
	gRenderer->SetPrimitiveColour( prim_colour );
}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_SetEnvColor( MicroCodeCommand command )
{
	DL_PF("    RGBA: %d %d %d %d", command.color.r, command.color.g, command.color.b, command.color.a);

	c32	env_colour( command.color.r, command.color.g,command.color.b, command.color.a );

	gRenderer->SetEnvColour( env_colour );
}

//*****************************************************************************
//RSP TRI commands..
//In HLE emulation you NEVER see these commands !
//*****************************************************************************
void DLParser_TriRSP( MicroCodeCommand command [[maybe_unused]] ){ DL_PF("    RSP Tri: (Ignored)"); }
