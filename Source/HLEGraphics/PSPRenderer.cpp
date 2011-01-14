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

//Draw normal filled triangles
#define DRAW_MODE GU_TRIANGLES
//Draw lines
//Also enable clean scene in advanced menu //Corn
//#define DRAW_MODE GU_LINE_STRIP

//If defined fog will be done by sceGU
//Otherwise it enables some non working legacy VFPU fog //Corn
#define NO_VFPU_FOG

#include "stdafx.h"

#include "PSPRenderer.h"
#include "Texture.h"
#include "TextureCache.h"
#include "RDP.h"
#include "RDPStateManager.h"
#include "BlendModes.h"
#include "DebugDisplayList.h"

#include "Combiner/RenderSettings.h"
#include "Combiner/BlendConstant.h"
#include "Combiner/CombinerTree.h"

#include "Graphics/NativeTexture.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/ColourValue.h"
#include "Graphics/PngUtil.h"

#include "Math/MathUtil.h"

#include "Debug/Dump.h"
#include "Debug/DBGConsole.h"

#include "Core/Memory.h"		// We access the memory buffers
#include "Core/ROM.h"

#include "OSHLE/ultra_gbi.h"
#include "OSHLE/ultra_os.h"		// System type

#include "Math/Math.h"			// VFPU Math

#include "Utility/Profiler.h"
#include "Utility/Preferences.h"
#include "Utility/IO.h"

#include <pspgu.h>
#include <pspgum.h>
#include <psputils.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspdisplay.h>

#include <vector>

#include "PushStructPack1.h"

#include "PopStructPack.h"

extern SImageDescriptor g_CI;		// XXXX SImageDescriptor g_CI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };
extern SImageDescriptor g_DI;		// XXXX SImageDescriptor g_DI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };

extern "C"
{
void	_TransformVerticesWithLighting_f0_t0( const Matrix4x4 * world_matrix, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out, u32 num_vertices, const TnLParams * params, const DaedalusLight * p_lights, u32 num_lights );
void	_TransformVerticesWithLighting_f0_t1( const Matrix4x4 * world_matrix, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out, u32 num_vertices, const TnLParams * params, const DaedalusLight * p_lights, u32 num_lights );
void	_TransformVerticesWithLighting_f0_t2( const Matrix4x4 * world_matrix, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out, u32 num_vertices, const TnLParams * params, const DaedalusLight * p_lights, u32 num_lights );
void	_TransformVerticesWithLighting_f1_t0( const Matrix4x4 * world_matrix, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out, u32 num_vertices, const TnLParams * params, const DaedalusLight * p_lights, u32 num_lights );
void	_TransformVerticesWithLighting_f1_t1( const Matrix4x4 * world_matrix, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out, u32 num_vertices, const TnLParams * params, const DaedalusLight * p_lights, u32 num_lights );
void	_TransformVerticesWithLighting_f1_t2( const Matrix4x4 * world_matrix, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out, u32 num_vertices, const TnLParams * params, const DaedalusLight * p_lights, u32 num_lights );

void	_TransformVerticesWithColour_f0_t0( const Matrix4x4 * world_matrix, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out, u32 num_vertices, const TnLParams * params );
void	_TransformVerticesWithColour_f0_t1( const Matrix4x4 * world_matrix, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out, u32 num_vertices, const TnLParams * params );
void	_TransformVerticesWithColour_f1_t0( const Matrix4x4 * world_matrix, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out, u32 num_vertices, const TnLParams * params );
void	_TransformVerticesWithColour_f1_t1( const Matrix4x4 * world_matrix, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out, u32 num_vertices, const TnLParams * params );

void	_ConvertVertices( DaedalusVtx * dest, const DaedalusVtx4 * source, u32 num_vertices );
void	_ConvertVerticesIndexed( DaedalusVtx * dest, const DaedalusVtx4 * source, u32 num_vertices, const u16 * indices );

u32		_ClipToHyperPlane( DaedalusVtx4 * dest, const DaedalusVtx4 * source, const v4 * plane, u32 num_verts );
}

// Bits for clipping
// +-+-+-
// xxyyzz

// NB: These are ordered such that the VFPU can generate them easily - make sure you keep the VFPU code up to date if changing these.
#define X_NEG  0x01
#define Y_NEG  0x02
#define Z_NEG  0x04
#define X_POS  0x08
#define Y_POS  0x10
#define Z_POS  0x20

// Test all but Z_NEG (for No Near Plane microcodes)
//static const u32 CLIP_TEST_FLAGS( X_POS | X_NEG | Y_POS | Y_NEG | Z_POS );
static const u32 CLIP_TEST_FLAGS( X_POS | X_NEG | Y_POS | Y_NEG | Z_POS | Z_NEG );

#define GL_TRUE                           1
#define GL_FALSE                          0

#undef min
#undef max

enum CycleType
{
	CYCLE_1CYCLE = 0,		// Please keep in this order - matches RDP
	CYCLE_2CYCLE,
	CYCLE_COPY,
	CYCLE_FILL,
};

extern u32 SCR_WIDTH;
extern u32 SCR_HEIGHT;

static f32 fViWidth = 320.0f;
static f32 fViHeight = 240.0f;
//static u32 uViWidth = 320;
//static u32 uViHeight = 240;

f32 gZoomX=1.0;	//Default is 1.0f

static const float gTexRectDepth( 0.0f );

bool bStarOrigin = false;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST	
// General purpose variable used for debugging
f32 TEST_VARX = 0.0f;
f32 TEST_VARY = 0.0f;		

ALIGNED_GLOBAL(u32,gWhiteTexture[gPlaceholderTextureWidth * gPlaceholderTextureHeight ], DATA_ALIGN);
ALIGNED_GLOBAL(u32,gPlaceholderTexture[gPlaceholderTextureWidth * gPlaceholderTextureHeight ], DATA_ALIGN);
ALIGNED_GLOBAL(u32,gSelectedTexture[gPlaceholderTextureWidth * gPlaceholderTextureHeight ], DATA_ALIGN);

extern void		PrintMux( FILE * fh, u64 mux );

//***************************************************************************
//*General blender used for testing //Corn
//***************************************************************************
u32 gTexInstall=1;
u32	gSetRGB=0;
u32	gSetA=0;
u32	gSetRGBA=0;
u32	gModA=0;
u32	gAOpaque=0;

u32	gsceENV=0;

u32	gTXTFUNC=6;	//defaults to replace

u32	gNumCyc=3;

u32	gForceRGB=6;	//defaults to magenta

const char *gForceColor[8] =
{
	"OFF",
	"White",
	"Black",
	"Red",
	"Green",
	"Blue",
	"Magenta",
	"Gold"
};

const char *gPSPtxtFunc[10] =
{
	"Modulate RGB",
	"Modulate RGBA",
	"Blend RGB",
	"Blend RGBA",
	"Add RGB",
	"Add RGBA",
	"Replace RGB",
	"Replace RGBA",
	"Decal RGB",
	"Decal RGBA"
};

const char *gCAdj[5] =
{
	"OFF",
	"Prim Color",
	"Prim Color Replicate Alpha",
	"Env Color",
	"Env Color Replicate Alpha",
};

#define BLEND_MODE_MAKER \
{ \
	const u32 PSPtxtFunc[5] = \
	{ \
		GU_TFX_MODULATE, \
		GU_TFX_BLEND, \
		GU_TFX_ADD, \
		GU_TFX_REPLACE, \
		GU_TFX_DECAL \
	}; \
	const u32 PSPtxtA[2] = \
	{ \
		GU_TCC_RGB, \
		GU_TCC_RGBA \
	}; \
	if( num_cycles & gNumCyc ) \
	{ \
		if( gForceRGB ) \
		{ \
			if( gForceRGB==1 ) details.ColourAdjuster.SetRGB( c32::White ); \
			else if( gForceRGB==2 ) details.ColourAdjuster.SetRGB( c32::Black ); \
			else if( gForceRGB==3 ) details.ColourAdjuster.SetRGB( c32::Red ); \
			else if( gForceRGB==4 ) details.ColourAdjuster.SetRGB( c32::Green ); \
			else if( gForceRGB==5 ) details.ColourAdjuster.SetRGB( c32::Blue ); \
			else if( gForceRGB==6 ) details.ColourAdjuster.SetRGB( c32::Magenta ); \
			else if( gForceRGB==7 ) details.ColourAdjuster.SetRGB( c32::Gold ); \
		} \
		if( gSetRGB ) \
		{ \
			if( gSetRGB==1 ) details.ColourAdjuster.SetRGB( details.PrimColour ); \
			else if( gSetRGB==2 ) details.ColourAdjuster.SetRGB( details.PrimColour.ReplicateAlpha() ); \
			else if( gSetRGB==3 ) details.ColourAdjuster.SetRGB( details.EnvColour ); \
			else if( gSetRGB==4 ) details.ColourAdjuster.SetRGB( details.EnvColour.ReplicateAlpha() ); \
		} \
		if( gSetA ) \
		{ \
			if( gSetA==1 ) details.ColourAdjuster.SetA( details.PrimColour ); \
			else if( gSetA==2 ) details.ColourAdjuster.SetA( details.PrimColour.ReplicateAlpha() ); \
			else if( gSetA==3 ) details.ColourAdjuster.SetA( details.EnvColour ); \
			else if( gSetA==4 ) details.ColourAdjuster.SetA( details.EnvColour.ReplicateAlpha() ); \
		} \
		if( gSetRGBA ) \
		{ \
			if( gSetRGBA==1 ) details.ColourAdjuster.SetRGBA( details.PrimColour ); \
			else if( gSetRGBA==2 ) details.ColourAdjuster.SetRGBA( details.PrimColour.ReplicateAlpha() ); \
			else if( gSetRGBA==3 ) details.ColourAdjuster.SetRGBA( details.EnvColour ); \
			else if( gSetRGBA==4 ) details.ColourAdjuster.SetRGBA( details.EnvColour.ReplicateAlpha() ); \
		} \
		if( gModA ) \
		{ \
			if( gModA==1 ) details.ColourAdjuster.ModulateA( details.PrimColour ); \
			else if( gModA==2 ) details.ColourAdjuster.ModulateA( details.PrimColour.ReplicateAlpha() ); \
			else if( gModA==3 ) details.ColourAdjuster.ModulateA( details.EnvColour ); \
			else if( gModA==4 ) details.ColourAdjuster.ModulateA( details.EnvColour.ReplicateAlpha() ); \
		} \
		if( gAOpaque ) details.ColourAdjuster.SetAOpaque(); \
		if( gsceENV ) \
		{ \
			if( gsceENV==1 ) sceGuTexEnvColor( details.EnvColour.GetColour() ); \
			else if( gsceENV==2 ) sceGuTexEnvColor( details.PrimColour.GetColour() ); \
		} \
		details.InstallTexture = gTexInstall; \
		sceGuTexFunc( PSPtxtFunc[ (gTXTFUNC >> 1) % 6 ], PSPtxtA[ gTXTFUNC & 1 ] ); \
	} \
} \

#endif

//*****************************************************************************
// Creator function for singleton
//*****************************************************************************
template<> bool CSingleton< PSPRenderer >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new PSPRenderer();
	return mpInstance != NULL;
}

//*****************************************************************************
//
//*****************************************************************************
PSPRenderer::PSPRenderer()
:	mN64ToPSPScale( 2.0f, 2.0f )
,	mN64ToPSPTranslate( 0.0f, 0.0f )
,	mTnLModeFlags( 0 )

,	m_dwNumLights(0)
,	m_bZBuffer(false)

,	m_bCull(true)
,	m_bCull_mode(GU_CCW)

,	mAlphaThreshold(0)

,	mSmooth( true )
,	mSmoothShade( true )

,	mPrimDepth( 0.0f )

,	mFogColour(0x00FFFFFF)

,	mProjectionTop(0)
,	mModelViewTop(0)
,	mWorldProjectValid(false)
,	mProjisNew(true)
,	mWPmodified(false)

,	m_dwNumIndices(0)
,	mVtxClipFlagsUnion( 0 )

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
,	m_dwNumTrisRendered( 0 )
,	m_dwNumTrisClipped( 0 )
,	mRecordCombinerStates( false )
#endif
{
	DAEDALUS_ASSERT( IsPointerAligned( &mTnLParams, 16 ), "Oops, params should be 16-byte aligned" );

	for ( u32 t = 0; t < NUM_N64_TEXTURES; t++ )
	{
		mTileTopLeft[t] = v2( 0.0f, 0.0f );
		mTileScale[t] = v2( 1.0f, 1.0f );
	}

	memset( mLights, 0, sizeof(mLights) );

	mTnLParams.Ambient = v4( 1.0f, 1.0f, 1.0f, 1.0f );
	mTnLParams.FogMult = 0.0f;
	mTnLParams.FogOffset = 0.0f;
	mTnLParams.TextureScaleX = 1.0f;
	mTnLParams.TextureScaleY = 1.0f;

	gRDPMux._u64 = 0;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	memset( gWhiteTexture, 0xff, sizeof(gWhiteTexture) );

	u32	texel_idx( 0 );
	const u32	COL_MAGENTA( c32::Magenta.GetColour() );
	const u32	COL_GREEN( c32::Green.GetColour() );
	const u32	COL_BLACK( c32::Black.GetColour() );
	for(u32 y = 0; y < gPlaceholderTextureHeight; ++y)
	{
		for(u32 x = 0; x < gPlaceholderTextureWidth; ++x)
		{
			gPlaceholderTexture[ texel_idx ] = ((x&1) == (y&1)) ? COL_MAGENTA : COL_BLACK;
			gSelectedTexture[ texel_idx ]    = ((x&1) == (y&1)) ? COL_GREEN   : COL_BLACK;

			texel_idx++;
		}
	}
#endif
	//
	//	Set up RGB = T0, A = T0
	//
	mCopyBlendStates = new CBlendStates;
	{
		CAlphaRenderSettings *	alpha_settings( new CAlphaRenderSettings( "Copy" ) );
		CRenderSettingsModulate *	colour_settings( new CRenderSettingsModulate( "Copy" ) );

		alpha_settings->AddTermTexel0();
		colour_settings->AddTermTexel0();

		mCopyBlendStates->SetAlphaSettings( alpha_settings );
		mCopyBlendStates->AddColourSettings( colour_settings );
	}


	//
	//	Set up RGB = Diffuse, A = Diffuse
	//
	mFillBlendStates = new CBlendStates;
	{
		CAlphaRenderSettings *	alpha_settings( new CAlphaRenderSettings( "Fill" ) );
		CRenderSettingsModulate *	colour_settings( new CRenderSettingsModulate( "Fill" ) );

		alpha_settings->AddTermConstant( new CBlendConstantExpressionValue( BC_SHADE ) );
		colour_settings->AddTermConstant(  new CBlendConstantExpressionValue( BC_SHADE ) );

		mFillBlendStates->SetAlphaSettings( alpha_settings );
		mFillBlendStates->AddColourSettings( colour_settings );
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline PSPRenderer::~PSPRenderer()
{
	delete mFillBlendStates;
	delete mCopyBlendStates;
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::RestoreRenderStates()
{
	// Initialise the device to our default state

	// We do our own culling
	sceGuDisable(GU_CULL_FACE);

	// But clip our tris please (looks better in far field see Aerogauge)
	sceGuEnable(GU_CLIP_PLANES);
	//sceGuDisable(GU_CLIP_PLANES);

	sceGuScissor(0,0, SCR_WIDTH,SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);

	// We do our own lighting
	sceGuDisable(GU_LIGHTING);

	sceGuAlphaFunc(GU_GEQUAL, 0x04, 0xff );
	sceGuEnable(GU_ALPHA_TEST);

	//sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	//sceGuEnable(GU_BLEND);
	sceGuDisable( GU_BLEND );

	// Default is ZBuffer disabled
	sceGuDepthMask(GL_TRUE);	// GL_TRUE to disable z-writes
	sceGuDepthFunc(GU_GEQUAL);		// GEQUAL?
	sceGuDisable(GU_DEPTH_TEST);

	// Initialise all the renderstate to our defaults.
	sceGuShadeModel(GU_SMOOTH);

	sceGuTexEnvColor( c32::White.GetColour() );
	sceGuTexOffset(0.0f,0.0f);

	//sceGuFog(near,far,mFogColour);
	// Texturing stuff
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);
	//sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexWrap(GU_REPEAT,GU_REPEAT); 

	//sceGuSetMatrix( GU_PROJECTION, reinterpret_cast< const ScePspFMatrix4 * >( &gMatrixIdentity ) );
	sceGuSetMatrix( GU_VIEW, reinterpret_cast< const ScePspFMatrix4 * >( &gMatrixIdentity ) );
	sceGuSetMatrix( GU_MODEL, reinterpret_cast< const ScePspFMatrix4 * >( &gMatrixIdentity ) );
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::SetVIScales()
{
	u32 width = Memory_VI_GetRegister( VI_WIDTH_REG );

	u32 ScaleX = Memory_VI_GetRegister( VI_X_SCALE_REG ) & 0xFFF;
	u32 ScaleY = Memory_VI_GetRegister( VI_Y_SCALE_REG ) & 0xFFF;

	f32 fScaleX = (f32)ScaleX / (1<<10);
	f32 fScaleY = (f32)ScaleY / (1<<10);

	u32 HStartReg = Memory_VI_GetRegister( VI_H_START_REG );
	u32 VStartReg = Memory_VI_GetRegister( VI_V_START_REG );

	u32	hstart = HStartReg >> 16;
	u32	hend = HStartReg & 0xffff;

	u32	vstart = VStartReg >> 16;
	u32	vend = VStartReg & 0xffff;

	fViWidth  =  (hend-hstart)    * fScaleX;
	fViHeight = ((vend-vstart)/2) * fScaleY;

	//If we are close to 240 in height then set to 240 //Corn
	if( abs(240 - fViHeight) < 4 ) 
		fViHeight = 240.0f;
	
	// XXX Need to check PAL games.
	//if(g_ROM.TvType != OS_TV_NTSC) sRatio = 9/11.0f;

	//This sets the correct height in various games ex : Megaman 64
	if( width > 0x300 )	
		fViHeight *= 2.0f;

	//Sometimes HStartReg and VStartReg are zero
	//This fixes gaps is some games ex: CyberTiger
	if( fViWidth < 100)
	{
		// Load Runner in game sets zero for fViWidth, but VI_WIDTH_REG sets 640..which is wrong
		//
		if( g_ROM.GameHacks == LOAD_RUNNER )
			fViWidth = 320.0f;
		else
			fViWidth = (f32)Memory_VI_GetRegister( VI_WIDTH_REG );
	}
	if( fViHeight < 100) 
	{
		fViHeight = fViWidth * 0.75f; //sRatio
	}

	//Used to set a limit on Scissors //Corn
	//uViWidth  = (u32)fViWidth - 1;
	//uViHeight = (u32)fViHeight - 1;
}

//*****************************************************************************
//
//*****************************************************************************
// Reset for a new frame

void	PSPRenderer::Reset()
{
	ResetMatrices();

	m_dwNumIndices = 0;
	mVtxClipFlagsUnion = 0;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	m_dwNumTrisRendered = 0;
	m_dwNumTrisClipped = 0;
#endif

}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::BeginScene()
{
	CGraphicsContext::Get()->BeginFrame();

	RestoreRenderStates();

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	mRecordedCombinerStates.clear();
#endif

	u32		display_width( 0 );
	u32		display_height( 0 );
	u32		frame_width( 480 );
	u32		frame_height( 272 );
	//
	//	We do this each frame as it lets us adapt to changes in the viewport dynamically
	//
	CGraphicsContext::Get()->ViewportType(&display_width, &display_height, &frame_width, &frame_height );

	DAEDALUS_ASSERT( display_width != 0 && display_height != 0, "Unhandled viewport type" );

	s32		display_x( (frame_width - display_width)/2 );
	s32		display_y( (frame_height - display_height)/2 );

	SetPSPViewport( display_x, display_y, display_width, display_height );

	v3 scale( 640.0f*0.25f, 480.0f*0.25f, 511.0f*0.25f );
	v3 trans( 640.0f*0.25f, 480.0f*0.25f, 511.0f*0.25f );

	SetN64Viewport( scale, trans );
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::EndScene()
{
	CGraphicsContext::Get()->EndFrame();

	//
	//	Clear this, to ensure we're force to check for updates to it on the next frame
	//	Not needed anymore??? //Corn
	/*for( u32 i = 0; i < NUM_N64_TEXTURES; i++ )
	{
		mpTexture[ i ] = NULL;
	}*/
}
#ifdef DAEDALUS_DEBUG_DISPLAYLIST	
//*****************************************************************************
//
//*****************************************************************************
void	PSPRenderer::SelectPlaceholderTexture( EPlaceholderTextureType type )
{
	switch( type )
	{
	case PTT_WHITE:			sceGuTexImage(0,gPlaceholderTextureWidth,gPlaceholderTextureHeight,gPlaceholderTextureWidth,gWhiteTexture); break;
	case PTT_SELECTED:		sceGuTexImage(0,gPlaceholderTextureWidth,gPlaceholderTextureHeight,gPlaceholderTextureWidth,gSelectedTexture); break;
	case PTT_MISSING:		sceGuTexImage(0,gPlaceholderTextureWidth,gPlaceholderTextureHeight,gPlaceholderTextureWidth,gPlaceholderTexture); break;
	default:
		DAEDALUS_ERROR( "Unhandled type" );
		break;
	}
}
#endif
//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::SetPSPViewport( s32 x, s32 y, u32 w, u32 h )
{
	mN64ToPSPScale.x = gZoomX * f32( w ) / fViWidth;
	mN64ToPSPScale.y = gZoomX * f32( h ) / fViHeight;

	mN64ToPSPTranslate.x  = f32( x - pspFpuRound(0.55f * (gZoomX - 1.0f) * fViWidth));
	mN64ToPSPTranslate.y  = f32( y - pspFpuRound(0.55f * (gZoomX - 1.0f) * fViHeight));

	UpdateViewport();
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::SetN64Viewport( const v3 & scale, const v3 & trans )
{
	mVpScale = scale;
	mVpTrans = trans;

	UpdateViewport();
}

//*****************************************************************************
//
//*****************************************************************************
void	PSPRenderer::UpdateViewport()
{
	u32		vx( 2048 );
	u32		vy( 2048 );

	v2		n64_min( mVpTrans.x - mVpScale.x, mVpTrans.y - mVpScale.y );
	v2		n64_max( mVpTrans.x + mVpScale.x, mVpTrans.y + mVpScale.y );

	v2		psp_min( ConvertN64ToPsp( n64_min ) );
	v2		psp_max( ConvertN64ToPsp( n64_max ) );

	s32		vp_x( s32( psp_min.x ) );
	s32		vp_y( s32( psp_min.y ) );
	s32		vp_width( s32( psp_max.x - psp_min.x ) );
	s32		vp_height( s32( psp_max.y - psp_min.y ) );

	sceGuOffset(vx - (vp_width/2),vy - (vp_height/2));
	sceGuViewport(vx + vp_x,vy + vp_y,vp_width,vp_height);
}

//*****************************************************************************
//
//*****************************************************************************
	// GE 007 and Megaman can act up on this
	// We round these value here, so that when we scale up the coords to our screen
	// coords we don't get any gaps.
	// Avoid temporary object
	//
#if 1
v2 PSPRenderer::ConvertN64ToPsp( const v2 & n64_coords ) const
{
	v2 answ;
	vfpu_N64_2_PSP( &answ.x, &n64_coords.x, &mN64ToPSPScale.x, &mN64ToPSPTranslate.x);
	return answ;
}
#else
inline v2 PSPRenderer::ConvertN64ToPsp( const v2 & n64_coords ) const
{
	return (v2 (pspFpuRound( pspFpuRound( n64_coords.x ) * mN64ToPSPScale.x + mN64ToPSPTranslate.x ), 
				pspFpuRound( pspFpuRound( n64_coords.y ) * mN64ToPSPScale.y + mN64ToPSPTranslate.y )));
}
#endif
//*****************************************************************************
//
//*****************************************************************************
PSPRenderer::SBlendStateEntry	PSPRenderer::LookupBlendState( u64 mux, bool two_cycles )
{
	DAEDALUS_PROFILE( "PSPRenderer::LookupBlendState" );
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	mRecordedCombinerStates.insert( mux );
#endif

	union
	{
		u64		key;
		u32		kpart[2];
	}un;

	un.key = mux;

	// Top 8 bits are never set - use the very top one to differentiate between 1/2 cycles
#if 1	//1->faster, 0->old
	un.kpart[1] |= (two_cycles << 31);
#else
	if( two_cycles ) un.key |= u64(1)<<63;
#endif

	BlendStatesMap::const_iterator	it( mBlendStatesMap.find( un.key ) );
	if( it != mBlendStatesMap.end() )
	{
		return it->second;
	}

	SBlendStateEntry			entry;
	CCombinerTree				tree( mux, two_cycles );
	entry.States = tree.GetBlendStates();


	// Only check for blendmodes when inexact blends are found, this is done to allow setting a default case to handle most (inexact) blends with GU_TFX_MODULATE
	// Only issue with this is that we won't be able to add custom blendmodes to non-innexact blends, but meh we shouldn't need to anyways. (this only happens with M64's star hack..)
	// A nice side effect is that it gives a slight speed up when changing scenes since we would only check for innexact blends and not every mux that gets passed here :) // Salvy
	//
	if( entry.States->IsInexact() )
	{
		entry.OverrideFunction = LookupOverrideBlendModeFunction( mux );
	}


	//if( entry.OverrideFunction == NULL ) // This won't happen anymore, so is disabled - Salvy
	{
		//CCombinerTree				tree( mux, two_cycles );
		//entry.States = tree.GetBlendStates();
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		printf( "Adding %08x%08x - %d cycles%s", u32(mux>>32), u32(mux), two_cycles ? 2 : 1, entry.States->IsInexact() ?  " - Inexact - bodging\n" : "\n");
#endif
	}

	mBlendStatesMap[ un.key ] = entry;
	return entry;
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::RenderUsingRenderSettings( const CBlendStates * states, DaedalusVtx * p_vertices, u32 num_vertices, u32 render_flags)
{
	DAEDALUS_PROFILE( "PSPRenderer::RenderUsingRenderSettings" );

	const CAlphaRenderSettings *	alpha_settings( states->GetAlphaSettings() );

	SRenderState	state;

	state.Vertices = p_vertices;
	state.NumVertices = num_vertices;
	state.PrimitiveColour = mPrimitiveColour;
	state.EnvironmentColour = mEnvColour;

	static std::vector< DaedalusVtx >	saved_verts;

	if( states->GetNumStates() > 1 )
	{
		saved_verts.resize( num_vertices );
		memcpy( &saved_verts[0], p_vertices, num_vertices * sizeof( DaedalusVtx ) );
	}


	for( u32 i = 0; i < states->GetNumStates(); ++i )
	{
		const CRenderSettings *		settings( states->GetColourSettings( i ) );

		bool		install_texture0( settings->UsesTexture0() || alpha_settings->UsesTexture0() );
		bool		install_texture1( settings->UsesTexture1() || alpha_settings->UsesTexture1() );

		SRenderStateOut out;

		memset( &out, 0, sizeof( out ) );

		settings->Apply( install_texture0 || install_texture1, state, out );
		alpha_settings->Apply( install_texture0 || install_texture1, state, out );

		// TODO: this nobbles the existing diffuse colour on each pass. Need to use a second buffer...
		if( i > 0 )
		{
			memcpy( p_vertices, &saved_verts[0], num_vertices * sizeof( DaedalusVtx ) );
		}

		if(out.VertexExpressionRGB != NULL)
		{
			out.VertexExpressionRGB->ApplyExpressionRGB( state );
		}
		if(out.VertexExpressionA != NULL)
		{
			out.VertexExpressionA->ApplyExpressionAlpha( state );
		}


		bool	installed_texture( false );

		if(install_texture0 || install_texture1)
		{
			u32	tfx( GU_TFX_MODULATE );
			switch( out.BlendMode )
			{
			case PBM_MODULATE:		tfx = GU_TFX_MODULATE; break;
			case PBM_REPLACE:		tfx = GU_TFX_REPLACE; break;
			case PBM_BLEND:			tfx = GU_TFX_BLEND; break;
			}

			u32 tcc( GU_TCC_RGBA );
			switch( out.BlendAlphaMode )
			{
			case PBAM_RGBA:			tcc = GU_TCC_RGBA; break;
			case PBAM_RGB:			tcc = GU_TCC_RGB; break;
			}

			sceGuTexFunc( tfx, tcc );
			if( tfx == GU_TFX_BLEND )
			{
				sceGuTexEnvColor( out.TextureFactor.GetColour() );
			}

			// NB if install_texture0 and install_texture1 are both set, 0 wins out
			u32		texture_idx( install_texture0 ? 0 : 1 );

			if( mpTexture[texture_idx] != NULL )
			{
				CRefPtr<CNativeTexture> texture;

				if(out.MakeTextureWhite)
				{
					texture = mpTexture[ texture_idx ]->GetRecolouredTexture( c32::White );
				}
				else
				{
					texture = mpTexture[ texture_idx ]->GetTexture();
				}

				if(texture != NULL)
				{
					texture->InstallTexture();
					installed_texture = true;
				}
			}
		}

		// If no texture was specified, or if we couldn't load it, clear it out
		if( !installed_texture )
		{
			sceGuDisable(GU_TEXTURE_2D);
		}

		sceGuDrawArray( DRAW_MODE, render_flags, num_vertices, NULL, p_vertices );
	}
}
extern u32 gOtherModeL;
extern void InitBlenderMode( u32 blender );
//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::RenderUsingCurrentBlendMode( DaedalusVtx * p_vertices, u32 num_vertices, ERenderMode mode, bool disable_zbuffer )
{
	//These has to be static!!! //Corn
	static u32	gLastRDPOtherMode	( 0 );
	static bool	gZFightingEnabled	= false;
	static bool	gLastUseZBuffer		= false;
	u32 blender	( gOtherModeL );

	DAEDALUS_PROFILE( "PSPRenderer::RenderUsingCurrentBlendMode" );

	if ( disable_zbuffer )
	{
		sceGuDisable(GU_DEPTH_TEST);
		sceGuDepthMask( GL_TRUE );	// GL_TRUE to disable z-writes
	}
	else
	{
		if ( (blender != gLastRDPOtherMode) || (m_bZBuffer != gLastUseZBuffer) )
		{
			// Only update if ZBuffer is enabled
			if (m_bZBuffer)
			{
				if(gRDPOtherMode.z_cmp || g_ROM.GameHacks == FZERO)
				{
					sceGuEnable(GU_DEPTH_TEST);
					
					// Fixes Zfighting issues we have on the PSP.
					if( IsZModeDecal() )
					{
						gZFightingEnabled = true;						
						sceGuDepthRange(65535,80);
					}
					else if( gZFightingEnabled )
					{
						gZFightingEnabled = false;						
						sceGuDepthRange(65535,0);
					}
 				}
				else
				{
					sceGuDisable(GU_DEPTH_TEST);
				}

				sceGuDepthMask( gRDPOtherMode.z_upd ? GL_FALSE : GL_TRUE );	// GL_TRUE to disable z-writes
			}
			else
			{
				sceGuDisable(GU_DEPTH_TEST);
				sceGuDepthMask( GL_TRUE );	// GL_TRUE to disable z-writes
			}

			gLastUseZBuffer = m_bZBuffer;
		}
	}

	gLastRDPOtherMode = blender;

	sceGuShadeModel( mSmooth ? GU_SMOOTH : GU_FLAT );
	//
	// Initiate Filter
	//
	//This sets our filtering either through gRDPOtherMode by default or we force it
	switch( gGlobalPreferences.ForceTextureFilter )
	{
		case FORCE_DEFAULT_FILTER:
			switch(gRDPOtherMode.text_filt)
			{
				case 2:			//G_TF_BILERP:	// 2
					sceGuTexFilter(GU_LINEAR,GU_LINEAR);
					break;
				default:		//G_TF_POINT:	// 0
					sceGuTexFilter(GU_NEAREST,GU_NEAREST);
					break;
			}
			break;
		case FORCE_POINT_FILTER:
			sceGuTexFilter(GU_NEAREST,GU_NEAREST);
			break;
		case FORCE_LINEAR_FILTER:
			sceGuTexFilter(GU_LINEAR,GU_LINEAR);
			break;
	}
	//
	// Initiate Blender
	//
	if( (blender & 0x4000) && (gRDPOtherMode.cycle_type < CYCLE_COPY) )
	{
		InitBlenderMode( blender >> 16 );
	}
	//
	// I can't think why the hand in mario's menu screen is rendered with an opaque rendermode,
	// and no alpha threshold. We set the alpha reference to 1 to ensure that the transparent pixels
	// don't get rendered. (We also do this in Super Smash Bothers to ensure transparent pixels
	// are not rendered. Also fixes other games - Kreationz). I hope this doesn't fuck anything up though. 
	//
#if 1	//1->Daedalus ALPHA, 0->Rice ALPHA //Corn
	if( gRDPOtherMode.alpha_compare == 0 )
	{
		if( gRDPOtherMode.cvg_x_alpha )	// I think this implies that alpha is coming from
		{
			bStarOrigin = true;	// Used to enable Mario 64 star blend when alpha origs, we do this to avoid messing the text.
			sceGuAlphaFunc(GU_GREATER, 0x70, 0xff); // Going over 0x70 brakes OOT, but going lesser than that makes lines on games visible...ex: Paper Mario.
			sceGuEnable(GU_ALPHA_TEST);
		}
		else
		{
			sceGuDisable(GU_ALPHA_TEST);
		}
	}
	else
	{
		if( gRDPOtherMode.alpha_cvg_sel && !gRDPOtherMode.cvg_x_alpha ) //We need cvg_sel for SSVN characters to display
		{
			// Use CVG for pixel alpha
			sceGuDisable(GU_ALPHA_TEST);
		}
		else
		{
			// G_AC_THRESHOLD || G_AC_DITHER
			sceGuAlphaFunc( mAlphaThreshold ? GU_GREATER : GU_GEQUAL, mAlphaThreshold, 0xff);
			sceGuEnable(GU_ALPHA_TEST);
		}
	}

#else
	if( gRDPOtherMode.alpha_compare == 0 )
	{
		if( gRDPOtherMode.cvg_x_alpha && ( gRDPOtherMode.alpha_cvg_sel || gRDPOtherMode.aa_en ) )
		{
			// Going over 0x70 brakes OOT, but going lesser than that makes lines on games visible...ex: Paper Mario.
			bStarOrigin = true;	// Used to enable Mario 64 star blend when alpha origs, we do this to avoid messing the text.
			sceGuAlphaFunc(GU_GREATER, 0x70, 0xff);
			sceGuEnable(GU_ALPHA_TEST);
		}
		else
		{
			sceGuDisable(GU_ALPHA_TEST);
		}
	}
    else if( gRDPOtherMode.alpha_compare == 3 )
		{
			//RDP_ALPHA_COMPARE_DITHER
			sceGuDisable(GU_ALPHA_TEST);
		}
    else if( gRDPOtherMode.alpha_cvg_sel && !gRDPOtherMode.cvg_x_alpha )
        {
            // Use CVG for pixel alpha
            sceGuDisable(GU_ALPHA_TEST);
        }
        else
        {
            // RDP_ALPHA_COMPARE_THRESHOLD || RDP_ALPHA_COMPARE_DITHER
			sceGuAlphaFunc( mAlphaThreshold ? GU_GREATER : GU_GEQUAL, mAlphaThreshold, 0xff);
			sceGuEnable(GU_ALPHA_TEST);
        }
#endif

	SBlendStateEntry		blend_entry;

	switch ( gRDPOtherMode.cycle_type )
	{
		case CYCLE_COPY:		blend_entry.States = mCopyBlendStates; break;
		case CYCLE_FILL:		blend_entry.States = mFillBlendStates; break;
		case CYCLE_1CYCLE:		blend_entry = LookupBlendState( gRDPMux._u64, false ); break;
		case CYCLE_2CYCLE:		blend_entry = LookupBlendState( gRDPMux._u64, true ); break;
	}

	u32		render_flags( DAEDALUS_VERTEX_FLAGS );

	switch( mode )
	{
	case RM_RENDER_2D:		render_flags |= GU_TRANSFORM_2D; break;
	case RM_RENDER_3D:		render_flags |= GU_TRANSFORM_3D; break;
	}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if(IsCombinerStateDisabled( gRDPMux._u64 ))
	{
	#if 1 //1->Allow Blend Explorer, 0->Nasty texture //Corn
		SBlendModeDetails		details;
		u32	num_cycles = gRDPOtherMode.cycle_type == CYCLE_2CYCLE ? 2 : 1;

		details.InstallTexture = true;
		details.EnvColour = mEnvColour;
		details.PrimColour = mPrimitiveColour;
		details.ColourAdjuster.Reset();
		details.RecolourTextureWhite = false;

		//Insert the Blend Explorer
		BLEND_MODE_MAKER

		bool	installed_texture( false );

		if( details.InstallTexture )
		{
			if( mpTexture[ 0 ] != NULL )
			{
				CRefPtr<CNativeTexture> texture;

				if(details.RecolourTextureWhite)
				{
					texture = mpTexture[ 0 ]->GetRecolouredTexture( c32::White );
				}
				else
				{
					texture = mpTexture[ 0 ]->GetTexture();
				}

				if(texture != NULL)
				{
					texture->InstallTexture();
					installed_texture = true;
				}
			}
		}

		// If no texture was specified, or if we couldn't load it, clear it out
		if( !installed_texture ) sceGuDisable( GU_TEXTURE_2D );
		details.ColourAdjuster.Process( p_vertices, num_vertices );
		sceGuDrawArray( DRAW_MODE, render_flags, num_vertices, NULL, p_vertices );
	
	#else
		// Use the nasty placeholder texture
		sceGuEnable(GU_TEXTURE_2D);
		SelectPlaceholderTexture( PTT_SELECTED );
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
		sceGuTexMode(GU_PSM_8888,0,0,GL_TRUE);		// maxmips/a2/swizzle = 0
		sceGuDrawArray( DRAW_MODE, render_flags, num_vertices, NULL, p_vertices );
	#endif
	}
	else
#endif
	// This check is for inexact blends which were handled either by a custom blendmode or auto blendmode thing
	// This will always return true now, we should remove the null check eventually - Salvy
	if( blend_entry.OverrideFunction != NULL )
	{

#ifdef DAEDALUS_DEBUG_DISPLAYLIST	
		bool	inexact( blend_entry.States->IsInexact() );

		// Only dump missing_mux when we awant to search for inexact blends aka HighlightInexactBlendModes is enabled.
		// Otherwise will dump lotsa of missing_mux even though is not needed since was handled correctly by auto blendmode thing - Salvy
		//
		if( inexact && gGlobalPreferences.HighlightInexactBlendModes)
		{
			if(mUnhandledCombinderStates.find( gRDPMux._u64 ) == mUnhandledCombinderStates.end())
			{
				char szFilePath[MAX_PATH+1];

				Dump_GetDumpDirectory(szFilePath, g_ROM.settings.GameName.c_str());

				IO::Path::Append(szFilePath, "missing_mux.txt");

				FILE * fh( fopen(szFilePath, mUnhandledCombinderStates.empty() ? "w" : "a") );
				if(fh != NULL)
				{
					PrintMux( fh, gRDPMux._u64 );
					fclose(fh);
				}

				mUnhandledCombinderStates.insert( gRDPMux._u64 );
			}
		}

		if(inexact && gGlobalPreferences.HighlightInexactBlendModes)
		{
			
			sceGuEnable( GU_TEXTURE_2D );
			sceGuTexMode( GU_PSM_8888, 0, 0, GL_TRUE );		// maxmips/a2/swizzle = 0

			// Use the nasty placeholder texture
			SelectPlaceholderTexture( PTT_MISSING );
			sceGuTexFunc( GU_TFX_REPLACE, GU_TCC_RGBA );
			sceGuDrawArray( DRAW_MODE, render_flags, num_vertices, NULL, p_vertices );
		}
		else
#endif
		{

			// Local vars for now
			SBlendModeDetails		details;

			details.InstallTexture = true;
			details.EnvColour = mEnvColour;
			details.PrimColour = mPrimitiveColour;
			details.ColourAdjuster.Reset();
			details.RecolourTextureWhite = false;

			blend_entry.OverrideFunction( gRDPOtherMode.cycle_type == CYCLE_2CYCLE ? 2 : 1, details );

			bool	installed_texture( false );

			if( details.InstallTexture )
			{
				if( mpTexture[ 0 ] != NULL )
				{
					CRefPtr<CNativeTexture> texture;

					if(details.RecolourTextureWhite)
					{
						texture = mpTexture[ 0 ]->GetRecolouredTexture( c32::White );
					}
					else
					{
						texture = mpTexture[ 0 ]->GetTexture();
					}

					if(texture != NULL)
					{
						texture->InstallTexture();
						installed_texture = true;
					}
				}
			}

			// If no texture was specified, or if we couldn't load it, clear it out
			if( !installed_texture )
			{
				sceGuDisable( GU_TEXTURE_2D );
			}

			details.ColourAdjuster.Process( p_vertices, num_vertices );

			sceGuDrawArray( DRAW_MODE, render_flags, num_vertices, NULL, p_vertices );
		}
	}
	else if( blend_entry.States != NULL )
	{
		RenderUsingRenderSettings( blend_entry.States, p_vertices, num_vertices, render_flags );
	}
	else
	{
		// Set default states
		DAEDALUS_ERROR( "Unhandled blend mode" );
		sceGuDisable( GU_TEXTURE_2D );
		sceGuDrawArray( DRAW_MODE, render_flags, num_vertices, NULL, p_vertices );
	}
}

//*****************************************************************************
//
//*****************************************************************************
namespace
{
inline	v2	GetEdgeForCycleMode( void )
	{
		v2 edge( 0.f, 0.f );

		//
		// In Fill/Copy mode the coordinates are inclusive (i.e. add 1.0f to the w/h)
		//
		switch ( gRDPOtherMode.cycle_type )
		{
			case CYCLE_COPY:			edge.x += 1.0f; edge.y += 1.0f;	break;
			case CYCLE_FILL:			edge.x += 1.0f; edge.y += 1.0f;	break;
			case CYCLE_1CYCLE:											break;
			case CYCLE_2CYCLE:											break;
		}

		return edge;
	}
}

//*****************************************************************************
// Used for TexRect, TexRectFlip, FillRect
//*****************************************************************************
void PSPRenderer::RenderTriangleList( const DaedalusVtx * p_verts, u32 num_verts, bool disable_zbuffer )
{
	DaedalusVtx*	p_vertices( (DaedalusVtx*)sceGuGetMemory(num_verts*sizeof(DaedalusVtx)) );
	memcpy( p_vertices, p_verts, num_verts*sizeof(DaedalusVtx));

	//sceGuSetMatrix( GU_PROJECTION, reinterpret_cast< const ScePspFMatrix4 * >( &gMatrixIdentity ) );
	RenderUsingCurrentBlendMode( p_vertices, num_verts, RM_RENDER_2D, disable_zbuffer );
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::TexRect( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0, const v2 & uv1 )
{
	EnableTexturing( tile_idx );

	v2 edge( GetEdgeForCycleMode() );
	v2 screen0( ConvertN64ToPsp( xy0 ) );
	v2 screen1( ConvertN64ToPsp( xy1 + edge ) );
	v2 tex_uv0( uv0 - mTileTopLeft[ 0 ] );
	v2 tex_uv1( uv1 - mTileTopLeft[ 0 ] );

	//DL_PF( "      Screen:  %f,%f -> %f,%f", screen0.x, screen0.y, screen1.x, screen1.y );
	//DL_PF( "      Texture: %f,%f -> %f,%f", tex_uv0.x, tex_uv0.y, tex_uv1.x, tex_uv1.y );

	DaedalusVtx trv[ 6 ];

	f32 depth = gRDPOtherMode.depth_source ? mPrimDepth : 0.0f;

	v3	positions[ 4 ] =
	{
		v3( screen0.x, screen0.y, depth ),
		v3( screen1.x, screen0.y, depth ),
		v3( screen1.x, screen1.y, depth ),
		v3( screen0.x, screen1.y, depth ),
	};
	v2	tex_coords[ 4 ] =
	{
		v2( tex_uv0.x, tex_uv0.y ),
		v2( tex_uv1.x, tex_uv0.y ),
		v2( tex_uv1.x, tex_uv1.y ),
		v2( tex_uv0.x, tex_uv1.y ),
	};

	trv[0] = DaedalusVtx( positions[ 1 ], 0xffffffff, tex_coords[ 1 ] );
	trv[1] = DaedalusVtx( positions[ 0 ], 0xffffffff, tex_coords[ 0 ] );
	trv[2] = DaedalusVtx( positions[ 2 ], 0xffffffff, tex_coords[ 2 ] );

	trv[3] = DaedalusVtx( positions[ 2 ], 0xffffffff, tex_coords[ 2 ] );
	trv[4] = DaedalusVtx( positions[ 0 ], 0xffffffff, tex_coords[ 0 ] );
	trv[5] = DaedalusVtx( positions[ 3 ], 0xffffffff, tex_coords[ 3 ] );

	RenderTriangleList( trv, 6, gRDPOtherMode.depth_source ? false : true );
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::TexRectFlip( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0, const v2 & uv1 )
{
	EnableTexturing( tile_idx );

	v2 edge( GetEdgeForCycleMode() );
	v2 screen0( ConvertN64ToPsp( xy0 ) );
	v2 screen1( ConvertN64ToPsp( xy1 + edge ) );
	v2 tex_uv0( uv0 - mTileTopLeft[ 0 ] );
	v2 tex_uv1( uv1 - mTileTopLeft[ 0 ] );

	//DL_PF( "      Screen:  %f,%f -> %f,%f", screen0.x, screen0.y, screen1.x, screen1.y );
	//DL_PF( "      Texture: %f,%f -> %f,%f", tex_uv0.x, tex_uv0.y, tex_uv1.x, tex_uv1.y );

	DaedalusVtx trv[ 6 ];

	v3	positions[ 4 ] =
	{
		v3( screen0.x, screen0.y, gTexRectDepth ),
		v3( screen1.x, screen0.y, gTexRectDepth ),
		v3( screen1.x, screen1.y, gTexRectDepth ),
		v3( screen0.x, screen1.y, gTexRectDepth ),
	};
	v2	tex_coords[ 4 ] =
	{
		v2( tex_uv0.x, tex_uv0.y ),
		v2( tex_uv0.x, tex_uv1.y ),		// In TexRect this is tex_uv1.x, tex_uv0.y
		v2( tex_uv1.x, tex_uv1.y ),
		v2( tex_uv1.x, tex_uv0.y ),		//tex_uv0.x	tex_uv1.y
	};

	trv[0] = DaedalusVtx( positions[ 1 ], 0xffffffff, tex_coords[ 1 ] );
	trv[1] = DaedalusVtx( positions[ 0 ], 0xffffffff, tex_coords[ 0 ] );
	trv[2] = DaedalusVtx( positions[ 2 ], 0xffffffff, tex_coords[ 2 ] );

	trv[3] = DaedalusVtx( positions[ 2 ], 0xffffffff, tex_coords[ 2 ] );
	trv[4] = DaedalusVtx( positions[ 0 ], 0xffffffff, tex_coords[ 0 ] );
	trv[5] = DaedalusVtx( positions[ 3 ], 0xffffffff, tex_coords[ 3 ] );

	RenderTriangleList( trv, 6, true );
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::FillRect( const v2 & xy0, const v2 & xy1, u32 color )
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if ( (gRDPOtherMode._u64 & 0xffff0000) == 0x5f500000 )	//Used by Wave Racer
	{
		// this blend mode is mem*0 + mem*1, so we don't need to render it... Very odd!
		DAEDALUS_ERROR("	mem*0 + mem*1 - skipped");
		return;
	}
#endif

	// Unless we support fb emulation, we can safetly skip here
	if( g_CI.Size != G_IM_SIZ_16b )	return;

	// This if for C&C - It might break other stuff (I'm not sure if we should allow alpha or not..)
	//color |= 0xff000000;

	//
	// In Fill/Copy mode the coordinates are inclusive (i.e. add 1.0f to the w/h)
	//
	v2 edge( GetEdgeForCycleMode() );
	v2 screen0( ConvertN64ToPsp( xy0 ) );
	v2 screen1( ConvertN64ToPsp( xy1 + edge ) );

	DL_PF( "      Screen:  %f,%f -> %f,%f", screen0.x, screen0.y, screen1.x, screen1.y );

	DaedalusVtx trv[ 6 ];

	v3	positions[ 4 ] =
	{
		v3( screen0.x, screen0.y, gTexRectDepth ),
		v3( screen1.x, screen0.y, gTexRectDepth ),
		v3( screen1.x, screen1.y, gTexRectDepth ),
		v3( screen0.x, screen1.y, gTexRectDepth ),
	};
	v2	tex_coords[ 4 ] =
	{
		v2( 0.f, 0.f ),
		v2( 1.f, 0.f ),
		v2( 1.f, 1.f ),
		v2( 0.f, 1.f ),
	};

	trv[0] = DaedalusVtx( positions[ 1 ], color, tex_coords[ 1 ] );
	trv[1] = DaedalusVtx( positions[ 0 ], color, tex_coords[ 0 ] );
	trv[2] = DaedalusVtx( positions[ 2 ], color, tex_coords[ 2 ] );

	trv[3] = DaedalusVtx( positions[ 2 ], color, tex_coords[ 2 ] );
	trv[4] = DaedalusVtx( positions[ 0 ], color, tex_coords[ 0 ] );
	trv[5] = DaedalusVtx( positions[ 3 ], color, tex_coords[ 3 ] );

	RenderTriangleList( trv, 6, true );
}

//*****************************************************************************
// Returns true if triangle visible and rendered, false otherwise
//*****************************************************************************
bool PSPRenderer::AddTri(u32 v0, u32 v1, u32 v2)
{
	//DAEDALUS_PROFILE( "PSPRenderer::AddTri" );

	u8	f0( mVtxProjected[v0].ClipFlags );
	u8	f1( mVtxProjected[v1].ClipFlags );
	u8	f2( mVtxProjected[v2].ClipFlags );

	if ( f0 & f1 & f2 & CLIP_TEST_FLAGS )
	{
		DL_PF("   Tri: %d,%d,%d (clipped)", v0, v1, v2);

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		++m_dwNumTrisClipped;
#endif
		return false;

	}
	else
	{
		DL_PF("   Tri: %d,%d,%d", v0, v1, v2);

		m_swIndexBuffer[ m_dwNumIndices++ ] = (u16)v0;
		m_swIndexBuffer[ m_dwNumIndices++ ] = (u16)v1;
		m_swIndexBuffer[ m_dwNumIndices++ ] = (u16)v2;

		mVtxClipFlagsUnion |= f0 | f1 | f2;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		++m_dwNumTrisRendered;
#endif
		return true;
	}
}

//*****************************************************************************
//
//*****************************************************************************
bool PSPRenderer::TestVerts( u32 v0, u32 vn ) const
{
	u32 flags = mVtxProjected[v0].ClipFlags;

//v0 and Vn already clipped to maximum 15 //Corn 
//	for ( u32 i = v0+1; i <= vn && i < MAX_VERTS; i++ )
	for ( u32 i = v0+1; i <= vn; i++ )
	{
		flags &= mVtxProjected[i].ClipFlags;
	}

	return (flags & CLIP_TEST_FLAGS) == 0;
}

//*****************************************************************************
//
//*****************************************************************************
inline v4 PSPRenderer::LightVert( const v3 & norm ) const
{
	// Do ambient
	v4	result( mTnLParams.Ambient );

	for ( s32 l = 0; l < m_dwNumLights; l++ )
	{
		f32 fCosT = norm.Dot( mLights[l].Direction );
		if (fCosT > 0.0f)
		{
			result.x += mLights[l].Colour.x * fCosT;
			result.y += mLights[l].Colour.y * fCosT;
			result.z += mLights[l].Colour.z * fCosT;
		//	result.w += mLights[l].Colour.w * fCosT;
		}
	}

	//Clamp to 1.0
	if( result.x > 1.0f ) result.x = 1.0f;
	if( result.y > 1.0f ) result.y = 1.0f;
	if( result.z > 1.0f ) result.z = 1.0f;
	if( result.w > 1.0f ) result.w = 1.0f;

	return result;
}

//*****************************************************************************
//
//	The following clipping code was taken from The Irrlicht Engine.
//	See http://irrlicht.sourceforge.net/ for more information.
//	Copyright (C) 2002-2006 Nikolaus Gebhardt/Alten Thomas
//
//*****************************************************************************
static const v4 NDCPlane[6] =
{
	v4(  0.f,  0.f, -1.f, -1.f ),	// near
	v4(  0.f,  0.f,  1.f, -1.f ),	// far
	v4(  1.f,  0.f,  0.f, -1.f ),	// left
	v4( -1.f,  0.f,  0.f, -1.f ),	// right
	v4(  0.f,  1.f,  0.f, -1.f ),	// bottom
	v4(  0.f, -1.f,  0.f, -1.f )	// top
};

// Ununsed... we might remove this, or keep it for check-in
#ifdef DAEDALUS_IS_LEGACY
//*****************************************************************************
//
//*****************************************************************************
void DaedalusVtx4::Interpolate( const DaedalusVtx4 & lhs, const DaedalusVtx4 & rhs, float factor )
{
	ProjectedPos = lhs.ProjectedPos + (rhs.ProjectedPos - lhs.ProjectedPos) * factor;
	TransformedPos = lhs.TransformedPos + (rhs.TransformedPos - lhs.TransformedPos) * factor;
	Colour = lhs.Colour + (rhs.Colour - lhs.Colour) * factor;
	Texture = lhs.Texture + (rhs.Texture - lhs.Texture) * factor;
	ClipFlags = 0;
}

//*****************************************************************************
//
//*****************************************************************************
static u32 clipToHyperPlane( DaedalusVtx4 * dest, const DaedalusVtx4 * source, u32 inCount, const v4 &plane )
{
	u32 outCount = 0;
	DaedalusVtx4 * out = dest;

	const DaedalusVtx4 * a;
	const DaedalusVtx4 * b = source;

	f32 bDotPlane;

	bDotPlane = b->ProjectedPos.Dot( plane );

	for( u32 i = 1; i < inCount + 1; ++i)
	{
		const s32 condition = i - inCount;
		const s32 index = (( ( condition >> 31 ) & ( i ^ condition ) ) ^ condition ) << 1;

		a = &source[ index ];

	
		// current point inside
		if ( a->ProjectedPos.Dot( plane ) <= 0.f )
		{
			// last point outside
			if ( bDotPlane > 0.f )
			{
				// intersect line segment with plane
				out->Interpolate( *b, *a, bDotPlane / (b->ProjectedPos - a->ProjectedPos).Dot( plane ) );
				out += 2;
				outCount += 1;
			}
			// copy current to out
			memcpy( out, a, sizeof( DaedalusVtx4 ) * 2);
			b = out;

			out += 2;
			outCount += 1;
		}
		else
		{
			// current point outside

			if ( bDotPlane <= 0.f )
			{
				// previous was inside
				// intersect line segment with plane
				out->Interpolate( *b, *a, bDotPlane / (b->ProjectedPos - a->ProjectedPos).Dot( plane ) );

				out += 2;
				outCount += 1;
			}
			b = a;
		}

		bDotPlane = b->ProjectedPos.Dot( plane );
	}

	return outCount;
}

u32 clip_tri_to_frustum( DaedalusVtx4 * v0, DaedalusVtx4 * v1 )
{
	u32 vOut( 3 );

	vOut = clipToHyperPlane( v1, v0, vOut, NDCPlane[0] ); if( vOut < 3 ) return 0;		// near
	vOut = clipToHyperPlane( v0, v1, vOut, NDCPlane[1] ); if( vOut < 3 ) return 0;		// left
	vOut = clipToHyperPlane( v1, v0, vOut, NDCPlane[2] ); if( vOut < 3 ) return 0;		// right
	vOut = clipToHyperPlane( v0, v1, vOut, NDCPlane[3] ); if( vOut < 3 ) return 0;		// bottom
	vOut = clipToHyperPlane( v1, v0, vOut, NDCPlane[4] ); if( vOut < 3 ) return 0;		// top
	vOut = clipToHyperPlane( v0, v1, vOut, NDCPlane[5] );		// far

	return vOut;
}
#endif	//DAEDALUS_IS_LEGACY

//*****************************************************************************
//
//*****************************************************************************
u32 clip_tri_to_frustum_vfpu( DaedalusVtx4 * v0, DaedalusVtx4 * v1 )
{
	u32 vOut( 3 );

	vOut = _ClipToHyperPlane( v1, v0, &NDCPlane[0], vOut ); if( vOut < 3 ) return 0;		// near
	vOut = _ClipToHyperPlane( v0, v1, &NDCPlane[1], vOut ); if( vOut < 3 ) return 0;		// left
	vOut = _ClipToHyperPlane( v1, v0, &NDCPlane[2], vOut ); if( vOut < 3 ) return 0;		// right
	vOut = _ClipToHyperPlane( v0, v1, &NDCPlane[3], vOut ); if( vOut < 3 ) return 0;		// bottom
	vOut = _ClipToHyperPlane( v1, v0, &NDCPlane[4], vOut ); if( vOut < 3 ) return 0;		// top
	vOut = _ClipToHyperPlane( v0, v1, &NDCPlane[5], vOut );		// far

	return vOut;
}

//*****************************************************************************
//
//*****************************************************************************
bool PSPRenderer::FlushTris()
{
	DAEDALUS_PROFILE( "PSPRenderer::FlushTris" );

	if ( m_dwNumIndices == 0 )
	{
		mVtxClipFlagsUnion = 0; // Avoid clipping non visible tris
		return true;
	}

	u32				num_vertices;
	DaedalusVtx *	p_vertices;

	if(gGlobalPreferences.SoftwareClipping && (mVtxClipFlagsUnion != 0))
	{
		PrepareTrisClipped( &p_vertices, &num_vertices );
	}
	else
	{
		PrepareTrisUnclipped( &p_vertices, &num_vertices );
	}

	// Hack for Conker BFD || no vertices to render? //Corn
	extern bool bConkerHideShadow;
	if( (bConkerHideShadow && (g_ROM.GameHacks == CONKER)) || num_vertices == 0)
	{
		DAEDALUS_ERROR("Warning: Hack for Conker shadow || No Vtx to render" );
		m_dwNumIndices = 0;
		mVtxClipFlagsUnion = 0;
		return true;
	}

	// Hack for Pilotwings 64
	static bool skipNext=false;
	if( g_ROM.GameHacks == PILOT_WINGS )
	{
		if ( (g_DI.Address == g_CI.Address) && gRDPOtherMode.z_cmp+gRDPOtherMode.z_upd > 0 )
		{
			DAEDALUS_ERROR("Warning: using Flushtris to write Zbuffer" );
			m_dwNumIndices = 0;
			mVtxClipFlagsUnion = 0;
			skipNext = true;
			return true;
		}
		else if( skipNext )
		{
			skipNext = false;
			m_dwNumIndices = 0;
			mVtxClipFlagsUnion = 0;
			return true;
		}	
	}
	
	//
	// Process the software vertex buffer to apply a couple of
	// necessary changes to the texture coords (this is required
	// because some ucodes set the texture after setting the vertices)
	//
	if (mTnLModeFlags & TNL_TEXTURE)
	{
		EnableTexturing( gTextureTile );

		// Bias points in decal mode
		// Is this for Z-fight? not working to well for that, at least not on PSP//Corn
		/*if (IsZModeDecal())
		{
			for ( u32 v = 0; v < num_vertices; v++ )
			{
				p_vertices[v].Position.z += 3.14;
			}
		}*/
	}

	//
	// Process the software vertex buffer to apply a couple of
	// necessary changes to the texture coords (this is required
	// because some ucodes set the texture after setting the vertices)
	//
	bool	update_tex_coords( (mTnLModeFlags & TNL_TEXTURE) != 0 && (mTnLModeFlags & (TNL_LIGHT|TNL_TEXGEN)) != (TNL_LIGHT|TNL_TEXGEN) );

	v2		offset( 0.0f, 0.0f );
	v2		scale( 1.0f, 1.0f );

	if( update_tex_coords )
	{
		offset = -mTileTopLeft[ 0 ];
		scale = mTileScale[ 0 ];
	}
	sceGuTexOffset( offset.x * scale.x, offset.y * scale.y );
	sceGuTexScale( scale.x, scale.y );


	//
	// If smooth shading is turned off, duplicate the first diffuse color for all three verts
	//
	//if ( !mSmoothShade )


	//
	//	If the depth source is prim, use the depth from the prim command
	//

	//
	//	For now do all clipping though ge -
	//	Some billboards (Koopa air) in Mario Kart have issues if Cull gets (re)moved
	if( m_bCull )
	{
		sceGuFrontFace(m_bCull_mode);
		sceGuEnable(GU_CULL_FACE);
	}
	else
	{
		sceGuDisable(GU_CULL_FACE);
	}

	//
	//	Render out our vertices
	//	XXXX Should be using GetWorldProject()?
	//
	const Matrix4x4 &	projection( mProjectionStack[mProjectionTop] );

	//If decal polys, we shift near/far plane a bit with projection matrix to eliminate z-fight //Corn
	//Works well in most games and is practically "free", however OOT does not work :(
	//if( (gRDPOtherMode.zmode == 3) && gRemoveZFighting ) projection.mRaw[10] += 0.0004f;

	const ScePspFMatrix4 *	p_psp_proj( reinterpret_cast< const ScePspFMatrix4 * >( &projection ) );
	
	sceGuSetMatrix( GU_PROJECTION, p_psp_proj );

	// Hack for nascar games..to be honest I don't know why these games are so different...might be tricky to have a proper fix..
	// Hack accuracy : works 100%
	//
	bool bIsZBuffer = (gRDPOtherMode.depth_source && g_ROM.GameHacks == NASCAR) ? true : false;

	RenderUsingCurrentBlendMode( p_vertices, num_vertices, RM_RENDER_3D, bIsZBuffer );

	sceGuDisable(GU_CULL_FACE);

	m_dwNumIndices = 0;
	mVtxClipFlagsUnion = 0;
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
namespace
{
	DaedalusVtx4		temp_a[ 8 ];
	DaedalusVtx4		temp_b[ 8 ];

	const u32			MAX_CLIPPED_VERTS = 1024;	// Probably excessively large...
	DaedalusVtx4		clipped_vertices[MAX_CLIPPED_VERTS];
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::PrepareTrisClipped( DaedalusVtx ** p_p_vertices, u32 * p_num_vertices ) const
{
	DAEDALUS_PROFILE( "PSPRenderer::PrepareTrisClipped" );

	//
	//	At this point all vertices are lit/projected and have both transformed and projected
	//	vertex positions. For the best results we clip against the projected vertex positions,
	//	but use the resulting intersections to interpolate the transformed positions.
	//	The clipping is more efficient in normalised device coordinates, but rendering these
	//	directly prevents the PSP performing perspective correction. We could invert the projection
	//	matrix and use this to back-project the clip planes into world coordinates, but this
	//	suffers from various precision issues. Carrying around both sets of coordinates gives
	//	us the best of both worlds :)
	//
	u32 num_vertices = 0;

	for(u32 i = 0; i < (m_dwNumIndices - 2);)
	{
		u32 idx0 = m_swIndexBuffer[ i++ ];
		u32 idx1 = m_swIndexBuffer[ i++ ];
		u32 idx2 = m_swIndexBuffer[ i++ ];

		if(mVtxProjected[idx0].ClipFlags | mVtxProjected[idx1].ClipFlags | mVtxProjected[idx2].ClipFlags)
		{
			temp_a[ 0 ] = mVtxProjected[ idx0 ];
			temp_a[ 1 ] = mVtxProjected[ idx1 ];
			temp_a[ 2 ] = mVtxProjected[ idx2 ];

			DL_PF( "i%d: %f,%f,%f,%f", i-3, temp_a[0].ProjectedPos.x, temp_a[0].ProjectedPos.y, temp_a[0].ProjectedPos.z, temp_a[0].ProjectedPos.w );
			DL_PF( "i%d: %f,%f,%f,%f", i-2, temp_a[1].ProjectedPos.x, temp_a[1].ProjectedPos.y, temp_a[1].ProjectedPos.z, temp_a[1].ProjectedPos.w );
			DL_PF( "i%d: %f,%f,%f,%f", i-1, temp_a[2].ProjectedPos.x, temp_a[2].ProjectedPos.y, temp_a[2].ProjectedPos.z, temp_a[2].ProjectedPos.w );

			u32 out = clip_tri_to_frustum_vfpu( temp_a, temp_b );
			if( out < 3 )
				continue;

			// Retesselate
			u32 new_num_vertices( num_vertices + (out - 3) * 3 );
			if( new_num_vertices > MAX_CLIPPED_VERTS )
			{
				DAEDALUS_ERROR( "Too many clipped verts: %d", new_num_vertices );
				break;
			}
			for( u32 j = 0; j <= out - 3; ++j)
			{
				clipped_vertices[ num_vertices++ ] = temp_a[ 0 ];
				clipped_vertices[ num_vertices++ ] = temp_a[ j + 1 ];
				clipped_vertices[ num_vertices++ ] = temp_a[ j + 2 ];
			}
		}
		else
		{
			if( num_vertices > (MAX_CLIPPED_VERTS - 3) )
			{
				DAEDALUS_ERROR( "Too many clipped verts: %d", num_vertices + 3 );
				break;
			}

			clipped_vertices[ num_vertices++ ] = mVtxProjected[ idx0 ];
			clipped_vertices[ num_vertices++ ] = mVtxProjected[ idx1 ];
			clipped_vertices[ num_vertices++ ] = mVtxProjected[ idx2 ];
		}
	}

	//
	//	Now the vertices have been clipped we need to write them into
	//	a buffer we obtain this from the display list.
	//  ToDo: Test Allocating vertex buffers to VRAM
	//	Maybe we should allocate all vertex buffers from VRAM?
	//
	DaedalusVtx *	p_vertices( (DaedalusVtx*)sceGuGetMemory(num_vertices*sizeof(DaedalusVtx)) );
	_ConvertVertices( p_vertices, clipped_vertices, num_vertices );
	*p_p_vertices = p_vertices;
	*p_num_vertices = num_vertices;
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::PrepareTrisUnclipped( DaedalusVtx ** p_p_vertices, u32 * p_num_vertices ) const
{
	DAEDALUS_PROFILE( "PSPRenderer::PrepareTrisUnclipped" );
	DAEDALUS_ASSERT( m_dwNumIndices > 0, "The number of indices should have been checked" );

	u32				num_vertices( m_dwNumIndices );
	DaedalusVtx *	p_vertices( (DaedalusVtx*)sceGuGetMemory(num_vertices*sizeof(DaedalusVtx)) );

	//
	//	Previously this code set up an index buffer to avoid processing the
	//	same vertices more than once - we avoid this now as there is apparently
	//	quite a large performance penalty associated with using these on the PSP.
	//
	//	http://forums.ps2dev.org/viewtopic.php?t=4703
	//
	//  ToDo: Test Allocating vertex buffers to VRAM
	//	ToDo: Why Indexed below?
	DAEDALUS_STATIC_ASSERT( MAX_CLIPPED_VERTS > ARRAYSIZE(m_swIndexBuffer) );
	_ConvertVerticesIndexed( p_vertices, mVtxProjected, num_vertices, m_swIndexBuffer );
	*p_p_vertices = p_vertices;
	*p_num_vertices = num_vertices;
}

// Ununsed... we might remove this, or keep it for check-in
#ifdef DAEDALUS_IS_LEGACY
//*****************************************************************************
//
//*****************************************************************************
static u32 CalcClipFlags( const v4 & projected )
{
	u32 clip_flags = 0;
	if		(-projected.x > projected.w)	clip_flags |= X_POS;
	else if (+projected.x > projected.w)	clip_flags |= X_NEG;

	if		(-projected.y > projected.w)	clip_flags |= Y_POS;
	else if (+projected.y > projected.w)	clip_flags |= Y_NEG;

	if		(-projected.z > projected.w)	clip_flags |= Z_POS;
	else if (+projected.z > projected.w)	clip_flags |= Z_NEG;

	return clip_flags;
}

void PSPRenderer::TestVFPUVerts( u32 v0, u32 num, const FiddledVtx * verts, const Matrix4x4 & mat_world )
{
	bool	env_map( (mTnLModeFlags & (TNL_LIGHT|TNL_TEXGEN)) == (TNL_LIGHT|TNL_TEXGEN) );

	u32 vend( v0 + num );
	for (u32 i = v0; i < vend; i++)
	{
		const FiddledVtx & vert = verts[i - v0];
		const v4 &	projected( mVtxProjected[i].ProjectedPos );

		if (mTnLModeFlags & TNL_FOG)
		{
			float eyespace_z = projected.z / projected.w;
			float fog_coeff = (eyespace_z * mTnLParams.FogMult) + mTnLParams.FogOffset;

			// Set the alpha
			f32 value = Clamp< f32 >( fog_coeff, 0.0f, 1.0f );

			if( pspFpuAbs( value - mVtxProjected[i].Colour.w ) > 0.01f )
			{
				printf( "Fog wrong: %f != %f\n", mVtxProjected[i].Colour.w, value );
			}
		}

		if (mTnLModeFlags & TNL_TEXTURE)
		{
			// Update texture coords n.b. need to divide tu/tv by bogus scale on addition to buffer

			// If the vert is already lit, then there is no normal (and hence we
			// can't generate tex coord)
			float tx, ty;
			if (env_map)
			{
				v3 vecTransformedNormal;		// Used only when TNL_LIGHT set
				v3	model_normal(f32( vert.norm_x ), f32( vert.norm_y ), f32( vert.norm_z ) );

				vecTransformedNormal = mat_world.TransformNormal( model_normal );
				vecTransformedNormal.Normalise();

				const v3 & norm = vecTransformedNormal;

				// Assign the spheremap's texture coordinates
				tx = (0.5f * ( 1.0f + ( norm.x*mat_world.m11 +
										norm.y*mat_world.m21 +
										norm.z*mat_world.m31 ) ));

				ty = (0.5f * ( 1.0f - ( norm.x*mat_world.m12 +
										norm.y*mat_world.m22 +
										norm.z*mat_world.m32 ) ));
			}
			else
			{
				tx = (float)vert.tu * mTnLParams.TextureScaleX;
				ty = (float)vert.tv * mTnLParams.TextureScaleY;
			}

			if( pspFpuAbs(tx - mVtxProjected[i].Texture.x ) > 0.0001f ||
				pspFpuAbs(ty - mVtxProjected[i].Texture.y ) > 0.0001f )
			{
				printf( "tx/y wrong : %f,%f != %f,%f (%s)\n", mVtxProjected[i].Texture.x, mVtxProjected[i].Texture.y, tx, ty, env_map ? "env" : "scale" );
			}
		}

		//
		//	Initialise the clipping flags (always done on the VFPU, so skip here)
		//
		u32 flags = CalcClipFlags( projected );
		if( flags != mVtxProjected[i].ClipFlags )
		{
			printf( "flags wrong: %02x != %02x\n", mVtxProjected[i].ClipFlags, flags );
		}
	}
}

template< bool FogEnable, int TextureMode >
void PSPRenderer::ProcessVerts( u32 v0, u32 num, const FiddledVtx * verts, const Matrix4x4 & mat_world )
{
	u32 vend( v0 + num );
	for (u32 i = v0; i < vend; i++)
	{
		const FiddledVtx & vert = verts[i - v0];
		const v4 &	projected( mVtxProjected[i].ProjectedPos );

		if (FogEnable)
		{
			float	fog_coeff;
			//if(fabsf(projected.w) > 0.0f)
			{
				float eyespace_z = projected.z / projected.w;
				fog_coeff = (eyespace_z * mTnLParams.FogMult) + mTnLParams.FogOffset;
			}
			//else
			//{
			//	fog_coeff = m_fFogOffset;
			//}

			// Set the alpha
			mVtxProjected[i].Colour.w = Clamp< f32 >( fog_coeff, 0.0f, 1.0f );
		}

		if (TextureMode == 2)
		{
			// Update texture coords n.b. need to divide tu/tv by bogus scale on addition to buffer

			// If the vert is already lit, then there is no normal (and hence we
			// can't generate tex coord)
			v3 vecTransformedNormal;		// Used only when TNL_LIGHT set
			v3	model_normal(f32( vert.norm_x ), f32( vert.norm_y ), f32( vert.norm_z ) );

			vecTransformedNormal = mat_world.TransformNormal( model_normal );
			vecTransformedNormal.Normalise();

			const v3 & norm = vecTransformedNormal;

			// Assign the spheremap's texture coordinates
			mVtxProjected[i].Texture.x = (0.5f * ( 1.0f + ( norm.x*mat_world.m11 +
														    norm.y*mat_world.m21 +
														    norm.z*mat_world.m31 ) ));

			mVtxProjected[i].Texture.y = (0.5f * ( 1.0f - ( norm.x*mat_world.m12 +
														    norm.y*mat_world.m22 +
														    norm.z*mat_world.m32 ) ));
		}
		else if(TextureMode == 1)
		{
			mVtxProjected[i].Texture.x = (float)vert.tu * mTnLParams.TextureScaleX;
			mVtxProjected[i].Texture.y = (float)vert.tv * mTnLParams.TextureScaleY;
			//mVtxProjected[i].t1 = mVtxProjected[i].t0;
		}

		//
		//	Initialise the clipping flags (always done on the VFPU, so skip here)
		//
		mVtxProjected[i].ClipFlags = CalcClipFlags( projected );
	}
}
#endif	//DAEDALUS_IS_LEGACY
//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::SetNewVertexInfoVFPU(u32 address, u32 v0, u32 n)
{
	const FiddledVtx * const pVtxBase( (const FiddledVtx*)(g_pu8RamBase + address) );

	const Matrix4x4 & matWorldProject( GetWorldProject() );

	//If WoldProjectmatrix has modified due to insert matrix
	//we need to update our modelView (fixes NMEs in Kirby and SSB) //Corn
	if( mWPmodified )
	{
		mWPmodified = false;
		
		//Only calculate inverse if there is a new Projectmatrix
		if( mProjisNew )
		{
			mProjisNew = false;
			mInvProjection = mProjectionStack[mProjectionTop].Inverse();
		}
		
		mModelViewStack[mModelViewTop] = mWorldProject * mInvProjection;
	}

	const Matrix4x4 & matWorld( mModelViewStack[mModelViewTop] );

#ifdef NO_VFPU_FOG
	switch( mTnLModeFlags & (TNL_TEXTURE|TNL_TEXGEN|TNL_LIGHT) )
	{
		// TNL_TEXGEN is ignored when TNL_LIGHT is disabled
	case                                   0: _TransformVerticesWithColour_f0_t0( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams ); break;
	case                         TNL_TEXTURE: _TransformVerticesWithColour_f0_t1( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams ); break;
	case            TNL_TEXGEN              : _TransformVerticesWithColour_f0_t0( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams ); break;
	case            TNL_TEXGEN | TNL_TEXTURE: _TransformVerticesWithColour_f0_t1( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams ); break;

		// TNL_TEXGEN is ignored when TNL_TEXTURE is disabled
	case TNL_LIGHT                                     : _TransformVerticesWithLighting_f0_t0( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams, mLights, m_dwNumLights ); break;
	case TNL_LIGHT |                        TNL_TEXTURE: _TransformVerticesWithLighting_f0_t1( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams, mLights, m_dwNumLights ); break;
	case TNL_LIGHT |           TNL_TEXGEN              : _TransformVerticesWithLighting_f0_t0( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams, mLights, m_dwNumLights ); break;
	case TNL_LIGHT |           TNL_TEXGEN | TNL_TEXTURE: _TransformVerticesWithLighting_f0_t2( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams, mLights, m_dwNumLights ); break;

#else
	switch( mTnLModeFlags & (TNL_TEXTURE|TNL_TEXGEN|TNL_FOG|TNL_LIGHT) )
	{
		// TNL_TEXGEN is ignored when TNL_LIGHT is disabled
	case                                   0: _TransformVerticesWithColour_f0_t0( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams ); break;
	case                         TNL_TEXTURE: _TransformVerticesWithColour_f0_t1( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams ); break;
	case            TNL_TEXGEN              : _TransformVerticesWithColour_f0_t0( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams ); break;
	case            TNL_TEXGEN | TNL_TEXTURE: _TransformVerticesWithColour_f0_t1( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams ); break;
	case  TNL_FOG                           : _TransformVerticesWithColour_f1_t0( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams ); break;
	case  TNL_FOG |              TNL_TEXTURE: _TransformVerticesWithColour_f1_t1( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams ); break;
	case  TNL_FOG | TNL_TEXGEN              : _TransformVerticesWithColour_f1_t0( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams ); break;
	case  TNL_FOG | TNL_TEXGEN | TNL_TEXTURE: _TransformVerticesWithColour_f1_t1( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams ); break;

		// TNL_TEXGEN is ignored when TNL_TEXTURE is disabled
	case TNL_LIGHT                                     : _TransformVerticesWithLighting_f0_t0( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams, mLights, m_dwNumLights ); break;
	case TNL_LIGHT |                        TNL_TEXTURE: _TransformVerticesWithLighting_f0_t1( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams, mLights, m_dwNumLights ); break;
	case TNL_LIGHT |           TNL_TEXGEN              : _TransformVerticesWithLighting_f0_t0( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams, mLights, m_dwNumLights ); break;
	case TNL_LIGHT |           TNL_TEXGEN | TNL_TEXTURE: _TransformVerticesWithLighting_f0_t2( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams, mLights, m_dwNumLights ); break;
	case TNL_LIGHT | TNL_FOG                           : _TransformVerticesWithLighting_f1_t0( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams, mLights, m_dwNumLights ); break;
	case TNL_LIGHT | TNL_FOG |              TNL_TEXTURE: _TransformVerticesWithLighting_f1_t1( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams, mLights, m_dwNumLights ); break;
	case TNL_LIGHT | TNL_FOG | TNL_TEXGEN              : _TransformVerticesWithLighting_f1_t0( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams, mLights, m_dwNumLights ); break;
	case TNL_LIGHT | TNL_FOG | TNL_TEXGEN | TNL_TEXTURE: _TransformVerticesWithLighting_f1_t2( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnLParams, mLights, m_dwNumLights ); break;
#endif
	default:
		NODEFAULT;
		break;
	}

	//TestVFPUVerts( v0, n, pVtxBase, matWorld );
}
//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::SetNewVertexInfoConker(u32 address, u32 v0, u32 n)
{
	// Light not handled for Conker
	// Workaround until proper vtx is implemented
	//
	mTnLModeFlags&=~TNL_LIGHT;

	SetNewVertexInfoVFPU(address, v0, n);

}
//*****************************************************************************
// Assumes dwAddress has already been checked!
// Don't inline - it's too big with the transform macros
// DKR seems to use longer vert info
//*****************************************************************************
void PSPRenderer::SetNewVertexInfoDKR(u32 dwAddress, u32 dwV0, u32 dwNum)
{
	

#ifndef NO_VFPU_FOG
	s32 nFogR, nFogG, nFogB, nFogA;
	if (mTnLModeFlags & TNL_FOG)
	{
		nFogR = mFogColour.GetR();
		nFogG = mFogColour.GetG();
		nFogB = mFogColour.GetB();
		nFogA = mFogColour.GetA();
	}
#endif
	s16 * pVtxBase = (s16*)(g_pu8RamBase + dwAddress);

	const Matrix4x4 & matWorldProject( GetWorldProject() );

	u32 i;
	s32 nOff;

	nOff = 0;
	for (i = dwV0; i < dwV0 + dwNum; i++)
	{
		v4 w;
		u32 dwFlags;

		w.x = (float)pVtxBase[(  nOff) ^ 1];
		w.y = (float)pVtxBase[(++nOff) ^ 1];
		w.z = (float)pVtxBase[(++nOff) ^ 1];
		w.w = 1.0f;

		v4 & projected( mVtxProjected[i].ProjectedPos );
		projected = matWorldProject.Transform( w );	// Convert to w=1

		//float	rhw;
		//if(fabsf(projected.w) > 0.0f)
		//{
		//	rhw = 1.0f / ( projected.w );
		//}
		//else
		//{
		//	rhw = 1.0f;
		//}

		dwFlags = 0;

		// Clip
		mVtxProjected[i].ClipFlags = dwFlags;

		s16 wA = pVtxBase[(++nOff) ^ 1];
		s16 wB = pVtxBase[(++nOff) ^ 1];

		s8 r = (s8)(wA >> 8);
		s8 g = (s8)(wA);
		s8 b = (s8)(wB >> 8);
		s8 a = (s8)(wB);

		v3 vecTransformedNormal;
		v4 colour;

		if (mTnLModeFlags & TNL_LIGHT)
		{
			v3 mn;		// modelnorm
			const Matrix4x4 & matWorld( mModelViewStack[mModelViewTop] );

			mn.x = (float)r; //norma_x;
			mn.y = (float)g; //norma_y;
			mn.z = (float)b; //norma_z;

			vecTransformedNormal = matWorld.TransformNormal( mn );
			vecTransformedNormal.Normalise();

			colour = LightVert(vecTransformedNormal);
			colour.w = a * (1.0f / 255.0f);
		}
		else
		{
			colour = v4( r * (1.0f / 255.0f), g * (1.0f / 255.0f), b * (1.0f / 255.0f), a * (1.0f / 255.0f) );
		}

		// Assign true vert colour after lighting/fogging
		mVtxProjected[i].Colour = colour;

		/*if (mTnLModeFlags & TNL_TEXTURE)
		{
			// Update texture coords n.b. need to divide tu/tv by bogus scale on addition to buffer
			v2 & t = m_vecTexture[i];

			// If the vert is already lit, then there is no normal (and hence we
			// can't generate tex coord)
			if ((mTnLModeFlags & (TNL_LIGHT|TNL_TEXGEN)) == (TNL_LIGHT|TNL_TEXGEN))
			{
				const Matrix4x4 & matWV = mModelViewStack[mModelViewTop];
				v3 & norm = vecTransformedNormal;

				// Assign the spheremap's texture coordinates
				t.x = (0.5f * ( 1.0f + ( norm.x*matWV.m00 +
										 norm.y*matWV.m10 +
										 norm.z*matWV.m20 ) ));

				t.y = (0.5f * ( 1.0f - ( norm.x*matWV.m01 +
										 norm.y*matWV.m11 +
										 norm.z*matWV.m21 ) ));
			}
			else
			{
				t.x = (float)vert.t.x * mTnLParams.TextureScaleX;
				t.y = (float)vert.t.y * mTnLParams.TextureScaleY;
			}
		}*/

		++nOff;
	}

}

// Ununsed... we might remove this, or keep it for check-in
#ifdef DAEDALUS_IS_LEGACY
//*****************************************************************************
//
//*****************************************************************************

void PSPRenderer::SetNewVertexInfoCPU(u32 dwAddress, u32 dwV0, u32 dwNum)
{
	//DBGConsole_Msg(0, "In SetNewVertexInfo");
	FiddledVtx * pVtxBase = (FiddledVtx*)(g_pu8RamBase + dwAddress);

	const Matrix4x4 & matWorld( mModelViewStack[mModelViewTop] );
	const Matrix4x4 & matWorldProject( GetWorldProject() );

	// Transform and Project + Lighting
	// or
	// Transform and Project with Colour

	u32 i;
	for (i = dwV0; i < dwV0 + dwNum; i++)
	{
		const FiddledVtx & vert = pVtxBase[i - dwV0];

		v4		w( f32( vert.x ), f32( vert.y ), f32( vert.z ), 1.0f );

		mVtxProjected[i].ProjectedPos = matWorldProject.Transform( w );
		mVtxProjected[i].TransformedPos = matWorld.Transform( w );

		DL_PF( "p%d: (%f,%f,%f) -> (%f,%f,%f,%f)", i, w.x, w.y, w.z, mVtxProjected[i].ProjectedPos.x, mVtxProjected[i].ProjectedPos.y, mVtxProjected[i].ProjectedPos.z, mVtxProjected[i].ProjectedPos.w );

		if (mTnLModeFlags & TNL_LIGHT)
		{
			v3	model_normal(f32( vert.norm_x ), f32( vert.norm_y ), f32( vert.norm_z ) );

			v3 vecTransformedNormal;		// Used only when TNL_LIGHT set
			vecTransformedNormal = matWorld.TransformNormal( model_normal );
			vecTransformedNormal.Normalise();

			mVtxProjected[i].Colour = LightVert(vecTransformedNormal);
		}
		else
		{
			mVtxProjected[i].Colour = v4( vert.rgba_r * (1.0f / 255.0f), vert.rgba_g * (1.0f / 255.0f), vert.rgba_b * (1.0f / 255.0f), vert.rgba_a * (1.0f / 255.0f) );
		}
	}

	//
	//	Template processing function to hoist conditional checks out of loop
	//
	switch( mTnLModeFlags & (TNL_TEXTURE|TNL_TEXGEN|TNL_FOG|TNL_LIGHT) )
	{
		// TNL_TEXGEN is ignored when TNL_LIGHT is disabled
	case                                              0: ProcessVerts< false, 0 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case                                    TNL_TEXTURE: ProcessVerts< false, 1 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case                       TNL_TEXGEN              : ProcessVerts< false, 0 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case                       TNL_TEXGEN | TNL_TEXTURE: ProcessVerts< false, 1 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case             TNL_FOG                           : ProcessVerts< true,  0 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case             TNL_FOG |              TNL_TEXTURE: ProcessVerts< true,  1 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case             TNL_FOG | TNL_TEXGEN              : ProcessVerts< true,  0 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case             TNL_FOG | TNL_TEXGEN | TNL_TEXTURE: ProcessVerts< true,  1 >( dwV0, dwNum, pVtxBase, matWorld );	break;

		// TNL_TEXGEN is ignored when TNL_TEXTURE is disabled
	case TNL_LIGHT                                     : ProcessVerts< false, 0 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case TNL_LIGHT |                        TNL_TEXTURE: ProcessVerts< false, 1 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case TNL_LIGHT |           TNL_TEXGEN              : ProcessVerts< false, 0 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case TNL_LIGHT |           TNL_TEXGEN | TNL_TEXTURE: ProcessVerts< false, 2 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case TNL_LIGHT | TNL_FOG                           : ProcessVerts< true,  0 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case TNL_LIGHT | TNL_FOG |              TNL_TEXTURE: ProcessVerts< true,  1 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case TNL_LIGHT | TNL_FOG | TNL_TEXGEN              : ProcessVerts< true,  0 >( dwV0, dwNum, pVtxBase, matWorld );	break;
	case TNL_LIGHT | TNL_FOG | TNL_TEXGEN | TNL_TEXTURE: ProcessVerts< true,  2 >( dwV0, dwNum, pVtxBase, matWorld );	break;

	default:
		NODEFAULT;
		break;
	}
}
#endif	//DAEDALUS_IS_LEGACY
//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::ModifyVertexInfo(u32 whered, u32 vert, u32 val)
{
	switch ( whered )
	{
		case G_MWO_POINT_RGBA:
			{
				DL_PF("      Setting RGBA to 0x%08x", val);
				SetVtxColor( vert, c32( val ) );
			}
			break;

		case G_MWO_POINT_ST:
			{
				s16 tu = s16(val >> 16);
				s16 tv = s16(val & 0xFFFF);
				DL_PF( "      Setting tu/tv to %f, %f", tu/32.0f, tv/32.0f );
				SetVtxTextureCoord( vert, tu, tv );
			}
			break;

		case G_MWO_POINT_XYSCREEN:
			{
				if( g_ROM.GameHacks == TARZAN ) return;

				s16 x = (u16)(val >> 16) >> 2;
				s16 y = (u16)(val & 0xFFFF) >> 2;

				//x -= uViWidth / 2;
				//y = uViHeight / 2 - y;

				DL_PF("		Modify vert %d: x=%d, y=%d", vert, x, y);
				
#if 1
				// Megaman and other games
				SetVtxXY( vert, f32(x<<1) / fViWidth , f32(y<<1) / fViHeight );
#else
				u32 current_scale = Memory_VI_GetRegister(VI_X_SCALE_REG);
				if((current_scale&0xF) != 0 )
				{
					// Tarzan... I don't know why is so different...
					SetVtxXY( vert, f32(x) / fViWidth , f32(y) / fViHeight );
				}
				else
				{	
					// Megaman and other games
					SetVtxXY( vert, f32(x<<1) / fViWidth , f32(y<<1) / fViHeight );
				}
#endif
			}
			break;

		case G_MWO_POINT_ZSCREEN:
			{
				s32 z = val >> 16;
				DAEDALUS_ERROR( "      Setting ZScreen to 0x%08x", z );
				//Not sure about the scaling here //Corn
				//SetVtxZ( vert, (( (f32)z / 0x03FF ) + 0.5f ) / 2.0f );
				SetVtxZ( vert, (( (f32)z ) + 0.5f ) / 2.0f );
			}
			break;

		default:
			DBGConsole_Msg( 0, "ModifyVtx - Setting vert data 0x%02x, 0x%08x", whered, val );
			DL_PF( "      Setting unknown value: 0x%02x, 0x%08x", whered, val );
			break;
	}
}

//*****************************************************************************
//
//*****************************************************************************
inline void PSPRenderer::SetVtxColor( u32 vert, c32 color )
{
	DAEDALUS_ASSERT( vert < MAX_VERTS, " SetVtxColor : Reached max of verts");

	mVtxProjected[vert].Colour = color.GetColourV4();
}

//*****************************************************************************
//
//*****************************************************************************
inline void PSPRenderer::SetVtxZ( u32 vert, float z )
{
	DAEDALUS_ASSERT( vert < MAX_VERTS, " SetVtxZ : Reached max of verts");

#if 1
	mVtxProjected[vert].TransformedPos.z = z;
#else
	mVtxProjected[vert].ProjectedPos.z = z;

	mVtxProjected[vert].TransformedPos.x = x * mVtxProjected[vert].TransformedPos.w;
	mVtxProjected[vert].TransformedPos.y = y * mVtxProjected[vert].TransformedPos.w;
	mVtxProjected[vert].TransformedPos.z = z * mVtxProjected[vert].TransformedPos.w;
#endif
}

//*****************************************************************************
//
//*****************************************************************************
inline void PSPRenderer::SetVtxXY( u32 vert, float x, float y )
{
	DAEDALUS_ASSERT( vert < MAX_VERTS, " SetVtxXY : Reached max of verts %d",vert);

#if 1
	mVtxProjected[vert].TransformedPos.x = x;
	mVtxProjected[vert].TransformedPos.y = y;
#else
	mVtxProjected[vert].ProjectedPos.x = x;
	mVtxProjected[vert].ProjectedPos.y = y;

	mVtxProjected[vert].TransformedPos.x = x * mVtxProjected[vert].TransformedPos.w;
	mVtxProjected[vert].TransformedPos.y = y * mVtxProjected[vert].TransformedPos.w;
	mVtxProjected[vert].TransformedPos.z = mVtxProjected[vert].ProjectedPos.z * mVtxProjected[vert].TransformedPos.w;
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::SetLightCol(u32 light, u32 colour)
{
	mLights[light].Colour.x = (f32)((colour >> 24)&0xFF) * (1.0f / 255.0f);
	mLights[light].Colour.y = (f32)((colour >> 16)&0xFF) * (1.0f / 255.0f);
	mLights[light].Colour.z = (f32)((colour >>  8)&0xFF) * (1.0f / 255.0f);
	mLights[light].Colour.w = 1.0f;	// Ignore light alpha
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::SetLightDirection(u32 l, float x, float y, float z)
{
	v3		normal( x, y, z );
	normal.Normalise();

	mLights[l].Direction.x = normal.x;
	mLights[l].Direction.y = normal.y;
	mLights[l].Direction.z = normal.z;
	mLights[l].Padding0 = 0.0f;
}

//*****************************************************************************
// Init matrix stack to identity matrices
//*****************************************************************************
void PSPRenderer::ResetMatrices()
{
	Matrix4x4 mat;

	mat.SetIdentity();

	mProjectionTop = 0;
	mModelViewTop = 0;
	mProjectionStack[0] = mat;
	mModelViewStack[0] = mat;
	mWorldProjectValid = false;
}

//*****************************************************************************
//
//*****************************************************************************
void	PSPRenderer::EnableTexturing( u32 tile_idx )
{
	EnableTexturing( 0, tile_idx );

	// XXXX Not required for texrect etc?
#ifdef RDP_USE_TEXEL1

	if ( gRDPOtherMode.text_lod )
	{
		// LOD is enabled - use the highest detail texture in texel1
		EnableTexturing( 1, tile_idx );
	}
	else
	{
		// LOD is disabled - use two textures
		EnableTexturing( 1, tile_idx+1 );
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void	PSPRenderer::EnableTexturing( u32 index, u32 tile_idx )
{
	DAEDALUS_PROFILE( "PSPRenderer::EnableTexturing" );

	DAEDALUS_ASSERT( tile_idx < 8, "Invalid tile index %d", tile_idx );
	DAEDALUS_ASSERT( index < NUM_N64_TEXTURES, "Invalid texture index %d", index );

	const TextureInfo &		ti( gRDPStateManager.GetTextureDescriptor( tile_idx ) );

	//
	//	Initialise the wrapping/texture offset first, which can be set
	//	independently of the actual texture.
	//
	const RDP_Tile &		rdp_tile( gRDPStateManager.GetTile( tile_idx ) );
	const RDP_TileSize &	tile_size( gRDPStateManager.GetTileSize( tile_idx ) );

	//
	// Initialise the clamping state. When the mask is 0, it forces clamp mode.
	//
	int mode_u = (rdp_tile.clamp_s || rdp_tile.mask_s == 0) ? GU_CLAMP : GU_REPEAT;
	int mode_v = (rdp_tile.clamp_t || rdp_tile.mask_t == 0)	? GU_CLAMP : GU_REPEAT;

	//
	//	In CRDPStateManager::GetTextureDescriptor, we limit the maximum dimension of a
	//	texture to that define by the mask_s/mask_t value.
	//	It this happens, the tile size can be larger than the truncated width/height
	//	as the rom can set clamp_s/clamp_t to wrap up to a certain value, then clamp.
	//	We can't support both wrapping and clamping (without manually repeating a texture...)
	//	so we choose to prefer wrapping.
	//	The castle in the background of the first SSB level is a good example of this behaviour.
	//	It sets up a texture with a mask_s/t of 6/6 (64x64), but sets the tile size to
	//	256*128. clamp_s/t are set, meaning the texture wraps 4x and 2x.
	//
	if( tile_size.GetWidth()  > ti.GetWidth()  ) mode_u = GU_REPEAT;
	if( tile_size.GetHeight() > ti.GetHeight() ) mode_v = GU_REPEAT;

	sceGuTexWrap( mode_u, mode_v );

	// XXXX Double check this
	mTileTopLeft[ index ] = v2( f32( tile_size.left) * (1.0f / 4.0f), f32(tile_size.top)* (1.0f / 4.0f) );

	DL_PF( "     *Performing texture map load:" );
	DL_PF( "     *Address: 0x%08x, Pitch: %d, Format: %s, Size: %dbpp, %dx%d",
			ti.GetLoadAddress(), ti.GetPitch(),
			ti.GetFormatName(), ti.GetSizeInBits(),
			ti.GetWidth(), ti.GetHeight() );

	if( (mpTexture[ index ] != NULL) && (mpTexture[ index ]->GetTextureInfo() == ti) ) return;

	// Check for 0 width/height textures
	if( (ti.GetWidth() == 0) || (ti.GetHeight() == 0) )
	{
		DAEDALUS_DL_ERROR( "Loading texture with 0 width/height" );
	}
	else
	{
		CRefPtr<CTexture>	texture( CTextureCache::Get()->GetTexture( &ti ) );

		if( texture != NULL )
		{
			//
			//	Avoid update check and divides if the texture is already installed
			//
			if( texture != mpTexture[ index ] )
			{
				mpTexture[ index ] = texture;

				texture->UpdateIfNecessary();

				const CRefPtr<CNativeTexture> & native_texture( texture->GetTexture() );
				if( native_texture != NULL )
				{
					mTileScale[ index ] = native_texture->GetScale();
				}
			}
		}
	}
}
//*****************************************************************************
//
//*****************************************************************************
void	PSPRenderer::SetScissor( u32 x0, u32 y0, u32 x1, u32 y1 )
{
	//Clamp scissor to max N64 screen resolution //Corn
	//if( x1 > uViWidth )  x1 = uViWidth;
	//if( y1 > uViHeight ) y1 = uViHeight;

	v2		n64_coords_tl( x0, y0 );
	v2		n64_coords_br( x1, y1 );

	v2		psp_coords_tl( ConvertN64ToPsp( n64_coords_tl ) );
	v2		psp_coords_br( ConvertN64ToPsp( n64_coords_br ) );

	// N.B. Think the arguments are x0,y0,x1,y1, and not x,y,w,h as the docs describe
	//Clamp TOP and LEFT values to 0 if < 0 , needed for zooming //Corn
	//printf("%d %d %d %d\n", s32(psp_coords_tl.x),s32(psp_coords_tl.y),s32(psp_coords_br.x),s32(psp_coords_br.y));
	sceGuScissor( s32(psp_coords_tl.x) < 0 ? 0 : s32(psp_coords_tl.x), s32(psp_coords_tl.y) < 0 ? 0 : s32(psp_coords_tl.y),
				  s32(psp_coords_br.x), s32(psp_coords_br.y) );
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::SetProjection(const Matrix4x4 & mat, bool bPush, bool bReplace)
{

#if 0	//1-> show matrix, 0-> skip
	for(u32 i=0;i<4;i++) printf("%+9.3f ",mat.mRaw[i]);
	printf("\n");
	for(u32 i=4;i<8;i++) printf("%+9.3f ",mat.mRaw[i]);
	printf("\n");
	for(u32 i=8;i<12;i++) printf("%+9.3f ",mat.mRaw[i]);
	printf("\n");
	for(u32 i=12;i<16;i++) printf("%+9.3f ",mat.mRaw[i]);
	printf("\n\n");
#endif

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		DL_PF("Level = %d\n"
			" %#+12.7f %#+12.7f %#+12.7f %#+12.7f\n"
			" %#+12.7f %#+12.7f %#+12.7f %#+12.7f\n"
			" %#+12.7f %#+12.7f %#+12.7f %#+12.7f\n"
			" %#+12.7f %#+12.7f %#+12.7f %#+12.7f\n",
			mProjectionTop,
			mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
			mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
			mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
			mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
	}
#endif

	// Projection
	if (bPush)
	{
		if (mProjectionTop >= (MATRIX_STACK_SIZE-1))
			DBGConsole_Msg(0, "Pushing past proj stack limits! %d/%d", mProjectionTop, MATRIX_STACK_SIZE);
		else
			++mProjectionTop;

		if (bReplace)
			// Load projection matrix
			mProjectionStack[mProjectionTop] = mat;
		else
			mProjectionStack[mProjectionTop] = mat * mProjectionStack[mProjectionTop-1];
	}
	else
	{
		if (bReplace)
		{
			// Load projection matrix
			mProjectionStack[mProjectionTop] = mat;

			//Hack needed to show heart in OOT & MM
			//it renders at Z cordinate = 0.0f that gets clipped away.
			//so we translate them a bit along Z to make them stick :) //Corn
			//
			if((g_ROM.GameHacks == ZELDA_OOT) || (g_ROM.GameHacks == ZELDA_MM))
				mProjectionStack[mProjectionTop].mRaw[14] += 0.4f;
		}
		else
			mProjectionStack[mProjectionTop] = mat * mProjectionStack[mProjectionTop];
	}

	mProjisNew = true;	// Note when a new P-matrix has been loaded
	mWorldProjectValid = false;
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::SetWorldView(const Matrix4x4 & mat, bool bPush, bool bReplace)
{

#if 0	//1-> show matrix, 0-> skip
	for(u32 i=0;i<4;i++) printf("%+9.3f ",mat.mRaw[i]);
	printf("\n");
	for(u32 i=4;i<8;i++) printf("%+9.3f ",mat.mRaw[i]);
	printf("\n");
	for(u32 i=8;i<12;i++) printf("%+9.3f ",mat.mRaw[i]);
	printf("\n");
	for(u32 i=12;i<16;i++) printf("%+9.3f ",mat.mRaw[i]);
	printf("\n\n");
#endif

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		DL_PF("Level = %d\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n",
			mModelViewTop,
			mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
			mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
			mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
			mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
	}
#endif

	// ModelView
	if (bPush)
	{
		if (mModelViewTop >= (MATRIX_STACK_SIZE-1))
			DBGConsole_Msg(0, "Pushing past modelview stack limits! %d/%d", mModelViewTop, MATRIX_STACK_SIZE);
		else
			++mModelViewTop;

		// We should store the current projection matrix...
		if (bReplace)
		{
			// Load projection matrix
			mModelViewStack[mModelViewTop] = mat;
		}
		else			// Multiply projection matrix
		{
			mModelViewStack[mModelViewTop] = mat * mModelViewStack[mModelViewTop-1];
		}
	}
	else	// NoPush
	{
		if (bReplace)
		{
			// Load projection matrix
			mModelViewStack[mModelViewTop] = mat;
		}
		else
		{
			// Multiply projection matrix
			mModelViewStack[mModelViewTop] = mat * mModelViewStack[mModelViewTop];
		}
	}

	mWorldProjectValid = false;
}

//*****************************************************************************
//
//*****************************************************************************
inline Matrix4x4 & PSPRenderer::GetWorldProject() const
{
	if( !mWorldProjectValid )
	{
		mWorldProject = mModelViewStack[mModelViewTop] * mProjectionStack[mProjectionTop];
		mWorldProjectValid = true;
	}

	return mWorldProject;
}

//*****************************************************************************
//
//*****************************************************************************

void PSPRenderer::PrintActive()
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		const Matrix4x4 & mat( GetWorldProject() );
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
}

//*****************************************************************************
//
//*****************************************************************************

void PSPRenderer::Draw2DTexture( float imageX, float imageY, float frameX, float frameY, float imageW, float imageH, float frameW, float frameH)
{
	DAEDALUS_PROFILE( "PSPRenderer::Draw2DTexture" );
	TextureVtx *p_verts = (TextureVtx*)sceGuGetMemory(2*sizeof(TextureVtx));

	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthMask( GL_TRUE );
	sceGuShadeModel( GU_FLAT );

	sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);

	sceGuEnable(GU_BLEND);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);

	p_verts[0].pos.x = frameX*mN64ToPSPScale.x + mN64ToPSPTranslate.x; // (Frame X Offset * X Scale Factor) + Screen X Offset
	p_verts[0].pos.y = frameY*mN64ToPSPScale.y + mN64ToPSPTranslate.y; // (Frame Y Offset * Y Scale Factor) + Screen Y Offset
	p_verts[0].pos.z = 0;
	p_verts[0].t0    = v2(imageX, imageY);				   // Source coordinates

	p_verts[1].pos.x = p_verts[0].pos.x + (frameW * mN64ToPSPScale.x); // Translated X Offset + (Image Width  * X Scale Factor)
	p_verts[1].pos.y = p_verts[0].pos.y + (frameH * mN64ToPSPScale.y); // Translated Y Offset + (Image Height * Y Scale Factor)
	p_verts[1].pos.z = 0;
	p_verts[1].t0    = v2(imageW, imageH);				   // Source dimentions

	sceGuDrawArray( GU_SPRITES, GU_TEXTURE_32BITF|GU_VERTEX_32BITF|GU_TRANSFORM_2D, 2, 0, p_verts);
}

//*****************************************************************************
//Modify the WorldProject matrix, used by Kirby mostly //Corn
//*****************************************************************************
void PSPRenderer::InsertMatrix(u32 w0, u32 w1)
{
	f32 fraction;

	if( !mWorldProjectValid )
	{
		mWorldProject = mModelViewStack[mModelViewTop] * mProjectionStack[mProjectionTop];
		mWorldProjectValid = true;
	}

	u32 x = (w0 & 0x1F) >> 1;
	u32 y = x >> 2;
	x &= 3;

	if (w0 & 0x20)
	{
		//Change fraction part
		fraction = (w1 >> 16) / 65536.0f;
		mWorldProject.m[y][x] = (f32)(s32)mWorldProject.m[y][x];
		mWorldProject.m[y][x] += fraction;

		fraction = (w1 & 0xFFFF) / 65536.0f;
		mWorldProject.m[y][x+1] = (f32)(s32)mWorldProject.m[y][x+1];
		mWorldProject.m[y][x+1] += fraction;
	}
	else
	{
		//Change integer part
		fraction = (f32)fabs(mWorldProject.m[y][x] - (s32)mWorldProject.m[y][x]);
		mWorldProject.m[y][x]	= (f32)(s16)(w1 >> 16) + fraction;

		fraction = (f32)fabs(mWorldProject.m[y][x+1] - (s32)mWorldProject.m[y][x+1]);
		mWorldProject.m[y][x+1] = (f32)(s16)(w1 & 0xFFFF) + fraction;
	}

	mWPmodified = true;	//Mark that Worldproject matrix is changed

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (gDisplayListFile != NULL)
	{
		DL_PF(
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
			" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n",
			mWorldProject.m[0][0], mWorldProject.m[0][1], mWorldProject.m[0][2], mWorldProject.m[0][3],
			mWorldProject.m[1][0], mWorldProject.m[1][1], mWorldProject.m[1][2], mWorldProject.m[1][3],
			mWorldProject.m[2][0], mWorldProject.m[2][1], mWorldProject.m[2][2], mWorldProject.m[2][3],
			mWorldProject.m[3][0], mWorldProject.m[3][1], mWorldProject.m[3][2], mWorldProject.m[3][3]);
	}
#endif
}

//*****************************************************************************
//Replaces the WorldProject matrix //Corn
//*****************************************************************************
void PSPRenderer::ForceMatrix(const Matrix4x4 & mat)
{
#if 0	//1-> show matrix, 0-> skip
	for(u32 i=0;i<4;i++) printf("%+9.3f ",mat.mRaw[i]);
	printf("\n");
	for(u32 i=4;i<8;i++) printf("%+9.3f ",mat.mRaw[i]);
	printf("\n");
	for(u32 i=8;i<12;i++) printf("%+9.3f ",mat.mRaw[i]);
	printf("\n");
	for(u32 i=12;i<16;i++) printf("%+9.3f ",mat.mRaw[i]);
	printf("\n\n");
#endif

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

	//We use it to get back the modelview matrix since we need it for proper rendering on PSP//Corn
	//The inverted projection matrix for Tarzan
	const Matrix4x4	invTarzan(	0.838109861116815f, 0.0f, 0.0f, 0.0f,
								0.0f, -0.38386429506604247f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, -0.009950865175186414f,
								0.0f, 0.0f, 1.0f, 0.010049104096541923f );

	//The inverted projection matrix for Donald duck
	const Matrix4x4	invDonald(	0.6841395918423196f, 0.0f, 0.0f, 0.0f,
								0.0f, 0.5131073266595174f, 0.0f, 0.0f,
								0.0f, 0.0f, -0.01532359917019646f, -0.01532359917019646f,
								0.0f, 0.0f, -0.9845562638123093f, 0.015443736187690802f );

	//Some games have permanent project matrixes so we can save CPU by storing the inverse
	//If that fails we invert the top project matrix to figure out the model matrix //Corn
	if( g_ROM.GameHacks == TARZAN )
	{
		mModelViewStack[mModelViewTop] = mat * invTarzan;
	}
	else if( g_ROM.GameHacks == DONALD )
	{
		mModelViewStack[mModelViewTop] = mat * invDonald;
	}
	else
	{
		//Check if current projection matrix has changed
		//To avoid calculating the inverse more than once per frame
		if ( mProjisNew )
		{
			mProjisNew = false;
			mInvProjection = mProjectionStack[mProjectionTop].Inverse();
		}

		mModelViewStack[mModelViewTop] = mat * mInvProjection;
	}
	
	mWorldProject = mat;
	mWorldProjectValid = true;
}
//*****************************************************************************
//
//*****************************************************************************