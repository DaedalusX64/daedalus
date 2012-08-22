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

#include "stdafx.h"

#include "PSPRenderer.h"
#include "Texture.h"
#include "TextureCache.h"
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

//extern SImageDescriptor g_CI;		// XXXX SImageDescriptor g_CI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };
//extern SImageDescriptor g_DI;		// XXXX SImageDescriptor g_DI = { G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 0 };

extern "C"
{
void	_TnLVFPU( const Matrix4x4 * world_matrix, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out, u32 num_vertices, const TnLParams * params );
void	_TnLVFPUDKR( u32 num_vertices, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out );
void	_TnLVFPUDKRB( u32 num_vertices, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out );
void	_TnLVFPUCBFD( const Matrix4x4 * world_matrix, const Matrix4x4 * projection_matrix, const FiddledVtx * p_in, const DaedalusVtx4 * p_out, u32 num_vertices, const TnLParams * params, const s8 * model_norm , u32 v0 );
void	_TnLVFPUPD( const Matrix4x4 * world_matrix, const Matrix4x4 * projection_matrix, const FiddledVtxPD * p_in, const DaedalusVtx4 * p_out, u32 num_vertices, const TnLParams * params, const u8 * model_norm );

void	_ConvertVertice( DaedalusVtx * dest, const DaedalusVtx4 * source );
void	_ConvertVerticesIndexed( DaedalusVtx * dest, const DaedalusVtx4 * source, u32 num_vertices, const u16 * indices );

u32		_ClipToHyperPlane( DaedalusVtx4 * dest, const DaedalusVtx4 * source, const v4 * plane, u32 num_verts );
}

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

extern bool gRumblePakActive;

extern u32 SCR_WIDTH;
extern u32 SCR_HEIGHT;

extern u32 gAuxAddr;

static f32 fViWidth = 320.0f;
static f32 fViHeight = 240.0f;
u32 uViWidth = 320;
u32 uViHeight = 240;

f32 gZoomX=1.0;	//Default is 1.0f

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
u32 gTexInstall=1;	//defaults to texture on
u32	gSetRGB=0;
u32	gSetA=0;
u32	gSetRGBA=0;
u32	gModA=0;
u32	gAOpaque=0;

u32	gsceENV=0;

u32	gTXTFUNC=0;	//defaults to MODULATE_RGB

u32 gForceRGB=0;    //defaults to OFF

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
	switch( gForceRGB ) \
	{ \
		case 1: details.ColourAdjuster.SetRGB( c32::White ); break; \
		case 2: details.ColourAdjuster.SetRGB( c32::Black ); break; \
		case 3: details.ColourAdjuster.SetRGB( c32::Red ); break; \
		case 4: details.ColourAdjuster.SetRGB( c32::Green ); break; \
		case 5: details.ColourAdjuster.SetRGB( c32::Blue ); break; \
		case 6: details.ColourAdjuster.SetRGB( c32::Magenta ); break; \
		case 7: details.ColourAdjuster.SetRGB( c32::Gold ); break; \
	} \
	switch( gSetRGB ) \
	{ \
		case 1: details.ColourAdjuster.SetRGB( details.PrimColour ); break; \
		case 2: details.ColourAdjuster.SetRGB( details.PrimColour.ReplicateAlpha() ); break; \
		case 3: details.ColourAdjuster.SetRGB( details.EnvColour ); break; \
		case 4: details.ColourAdjuster.SetRGB( details.EnvColour.ReplicateAlpha() ); break; \
	} \
	switch( gSetA ) \
	{ \
		case 1: details.ColourAdjuster.SetA( details.PrimColour ); break; \
		case 2: details.ColourAdjuster.SetA( details.PrimColour.ReplicateAlpha() ); break; \
		case 3: details.ColourAdjuster.SetA( details.EnvColour ); break; \
		case 4: details.ColourAdjuster.SetA( details.EnvColour.ReplicateAlpha() ); break; \
	} \
	switch( gSetRGBA ) \
	{ \
		case 1: details.ColourAdjuster.SetRGBA( details.PrimColour ); break; \
		case 2: details.ColourAdjuster.SetRGBA( details.PrimColour.ReplicateAlpha() ); break; \
		case 3: details.ColourAdjuster.SetRGBA( details.EnvColour ); break; \
		case 4: details.ColourAdjuster.SetRGBA( details.EnvColour.ReplicateAlpha() ); break; \
	} \
	switch( gModA ) \
	{ \
		case 1: details.ColourAdjuster.ModulateA( details.PrimColour ); break; \
		case 2: details.ColourAdjuster.ModulateA( details.PrimColour.ReplicateAlpha() ); break; \
		case 3: details.ColourAdjuster.ModulateA( details.EnvColour ); break; \
		case 4: details.ColourAdjuster.ModulateA( details.EnvColour.ReplicateAlpha() ); break; \
	} \
	if( gAOpaque ) details.ColourAdjuster.SetAOpaque(); \
	switch( gsceENV ) \
	{ \
		case 1: sceGuTexEnvColor( details.EnvColour.GetColour() ); break; \
		case 2: sceGuTexEnvColor( details.PrimColour.GetColour() ); break; \
	} \
	details.InstallTexture = gTexInstall; \
	sceGuTexFunc( PSPtxtFunc[ (gTXTFUNC >> 1) % 6 ], PSPtxtA[ gTXTFUNC & 1 ] ); \
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

ViewportInfo	mView;
//*****************************************************************************
//
//*****************************************************************************
PSPRenderer::PSPRenderer()
:	mN64ToPSPScale( 2.0f, 2.0f )
,	mN64ToPSPTranslate( 0.0f, 0.0f )
,	mMux( 0 )

,	mTextureTile(0)

,	mAlphaThreshold(0)

,	mPrimDepth( 0.0f )

,	mFogColour(0x00FFFFFF)

,	mProjectionTop(0)
,	mModelViewTop(0)
,	mWorldProjectValid(false)
,	mReloadProj(true)
,	mWPmodified(false)

,	mNumIndices(0)
,	mVtxClipFlagsUnion( 0 )

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
,	m_dwNumTrisRendered( 0 )
,	m_dwNumTrisClipped( 0 )
,	m_dwNumRect( 0 )
,	mNastyTexture(false)
,	mRecordCombinerStates( false )
#endif
{
	DAEDALUS_ASSERT( IsPointerAligned( &mTnL, 16 ), "Oops, mTnL should be 16-byte aligned" );

	for ( u32 t = 0; t < NUM_N64_TEXTURES; t++ )
	{
		mTileTopLeft[t].x = 0.0f;
		mTileTopLeft[t].y = 0.0f;
		mTileScale[t].x = 1.0f;
		mTileScale[t].y = 1.0f;
		mTexWrap[t][0] = 0;
		mTexWrap[t][1] = 0;
	}

	mTnL.Flags._u32 = 0;
	mTnL.NumLights = 0;
	mTnL.TextureScaleX = 1.0f;
	mTnL.TextureScaleY = 1.0f;
	
	memset( mTnL.Lights, 0, sizeof(mTnL.Lights) );

	memset( &mView, 0, sizeof(mView) );

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

	// No fog
	sceGuDisable(GU_FOG);
		
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

	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuEnable(GU_BLEND);
	//sceGuDisable( GU_BLEND ); // Breaks Tarzan's text in menus

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

	f32 fScaleX = (f32)ScaleX / 1024.0f;
	f32 fScaleY = (f32)ScaleY / 2048.0f;

	u32 HStartReg = Memory_VI_GetRegister( VI_H_START_REG );
	u32 VStartReg = Memory_VI_GetRegister( VI_V_START_REG );

	u32	hstart = HStartReg >> 16;
	u32	hend = HStartReg & 0xffff;

	u32	vstart = VStartReg >> 16;
	u32	vend = VStartReg & 0xffff;

	// Sometimes HStartReg can be zero.. ex PD, Lode Runner, Cyber Tiger
	if (hend == hstart)
	{
		hend = (u32)(width / fScaleX);
	}

	fViWidth  =  (hend-hstart)    * fScaleX;
	fViHeight =  (vend-vstart)    * fScaleY * 1.0126582f;

	// XXX Need to check PAL games.
	//if(g_ROM.TvType != OS_TV_NTSC) sRatio = 9/11.0f;

	//This corrects height in various games ex : Megaman 64, CyberTiger
	if( width > 0x300 )	
	{
		fViHeight *= 2.0f;
	}

	//Used to set a limit on Scissors //Corn
	uViWidth  = (u32)fViWidth - 1;
	uViHeight = (u32)fViHeight - 1;
}

//*****************************************************************************
//
//*****************************************************************************
// Reset for a new frame

void	PSPRenderer::Reset()
{
	ResetMatrices();

	mNumIndices = 0;
	mVtxClipFlagsUnion = 0;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	m_dwNumTrisRendered = 0;
	m_dwNumTrisClipped = 0;
	m_dwNumRect = 0;
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

	// Update viewport only if user changed it in settings or vi register changed it
	// Both happen really rare
	//
	if( !mView.Update					&		//  We need to update after pause menu?
		(mView.ViWidth  == uViWidth)    &		//  VI register changed width? (bug fix GE007) 
		(mView.ViHeight == uViHeight)   &		//  VI register changed height?
		(mView.Rumble   == gRumblePakActive) )	//  RumblePak active? Don't bother to update if no rumble feedback too
	{
		return;
	}

	u32	display_width( 0 );
	u32 display_height( 0 );

	CGraphicsContext::Get()->ViewportType(&display_width, &display_height);

	DAEDALUS_ASSERT( display_width && display_height, "Unhandled viewport type" );

	u32 frame_width(  gGlobalPreferences.TVEnable ? 720 : 480 );
	u32	frame_height( gGlobalPreferences.TVEnable ? 480 : 272 );

	v2 scale( 640.0f*0.25f, 480.0f*0.25f );
	v2 trans( 640.0f*0.25f, 480.0f*0.25f );

	s32 display_x( (frame_width - display_width) / 2 );
	s32 display_y( (frame_height - display_height) / 2 );

	SetPSPViewport( display_x, display_y, display_width, display_height );
	SetN64Viewport( scale, trans );

	mView.Rumble	= gRumblePakActive;
	mView.Update	= false;
	mView.ViWidth	= uViWidth;
	mView.ViHeight	= uViHeight;
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::EndScene()
{
	CGraphicsContext::Get()->EndFrame();

	//
	//	Clear this, to ensure we're force to check for updates to it on the next frame
	for( u32 i = 0; i < NUM_N64_TEXTURES; i++ )
	{
		mpTexture[ i ] = NULL;
	}
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
	
	if( gRumblePakActive )
	{
	    mN64ToPSPTranslate.x += (pspFastRand() & 3);
		mN64ToPSPTranslate.y += (pspFastRand() & 3);
	}

	UpdateViewport();
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::SetN64Viewport( const v2 & scale, const v2 & trans )
{
	// Only Update viewport when it actually changed, this happens rarely 
	//
	if( mVpScale.x == scale.x && mVpScale.y == scale.y && 
		mVpTrans.x == trans.x && mVpTrans.y == trans.y )	
		return;

	mVpScale.x = scale.x;
	mVpScale.y = scale.y;

	mVpTrans.x = trans.x;
	mVpTrans.y = trans.y;
	
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

	v2		psp_min;
	v2		psp_max;
	ConvertN64ToPsp( n64_min, psp_min );
	ConvertN64ToPsp( n64_max, psp_max );

	s32		vp_x( s32( psp_min.x ) );
	s32		vp_y( s32( psp_min.y ) );
	s32		vp_width( s32( psp_max.x - psp_min.x ) );
	s32		vp_height( s32( psp_max.y - psp_min.y ) );

	//DBGConsole_Msg(0, "[WViewport Changed (%d) (%d)]",vp_width,vp_height );

	sceGuOffset(vx - (vp_width/2),vy - (vp_height/2));
	sceGuViewport(vx + vp_x,vy + vp_y,vp_width,vp_height);
}

//*****************************************************************************
//
//*****************************************************************************
// GE 007 and Megaman can act up on this
// We round these value here, so that when we scale up the coords to our screen
// coords we don't get any gaps.
//
#if 0
inline void PSPRenderer::ConvertN64ToPsp( const v2 & n64_coords, v2 & answ ) const
{
	vfpu_N64_2_PSP( &answ.x, &n64_coords.x, &mN64ToPSPScale.x, &mN64ToPSPTranslate.x);
}
#else
inline void PSPRenderer::ConvertN64ToPsp( const v2 & n64_coords, v2 & answ ) const
{
	answ.x = pspFpuRound( pspFpuRound( n64_coords.x ) * mN64ToPSPScale.x + mN64ToPSPTranslate.x ); 
	answ.y = pspFpuRound( pspFpuRound( n64_coords.y ) * mN64ToPSPScale.y + mN64ToPSPTranslate.y );
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
		struct
		{
			u32	L;
			u32	H;
		};
	}un;

	un.key = mux;

	// Top 8 bits are never set - use the very top one to differentiate between 1/2 cycles
	un.H |= (two_cycles << 31);

	BlendStatesMap::const_iterator	it( mBlendStatesMap.find( un.key ) );
	if( it != mBlendStatesMap.end() )
	{
		return it->second;
	}

	SBlendStateEntry			entry;
	CCombinerTree				tree( mux, two_cycles );
	entry.States = tree.GetBlendStates();


	// Only check for blendmodes when inexact blends are found, this is done to allow setting a default case to handle most (inexact) blends with GU_TFX_MODULATE
	// A nice side effect is that it gives a slight speed up when changing scenes since we would only check for innexact blends and not every mux that gets passed here :) // Salvy
	//
	if( entry.States->IsInexact() )
	{
		entry.OverrideFunction = LookupOverrideBlendModeInexact( mux );
	}
	else
	{
		// This is for non-inexact blends, errg hacks and such to be more precise
		entry.OverrideFunction = LookupOverrideBlendModeForced( mux );
	}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST	
	if( entry.OverrideFunction == NULL )
	{
		//CCombinerTree				tree( mux, two_cycles );
		//entry.States = tree.GetBlendStates();
		printf( "Adding %08x%08x - %d cycles%s", u32(mux>>32), u32(mux), two_cycles ? 2 : 1, entry.States->IsInexact() ?  " - Inexact - bodging\n" : "\n");

	}
#endif
	mBlendStatesMap[ un.key ] = entry;
	return entry;
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::RenderUsingRenderSettings( const CBlendStates * states, DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_flags)
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

		bool install_texture0( settings->UsesTexture0() || alpha_settings->UsesTexture0() );
		bool install_texture1( settings->UsesTexture1() || alpha_settings->UsesTexture1() );

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


		bool installed_texture( false );

		u32 texture_idx( 0 );

		if(install_texture0 || install_texture1)
		{
			u32	tfx( GU_TFX_MODULATE );
			switch( out.BlendMode )
			{
			case PBM_MODULATE:		tfx = GU_TFX_MODULATE; break;
			case PBM_REPLACE:		tfx = GU_TFX_REPLACE; break;
			case PBM_BLEND:			tfx = GU_TFX_BLEND; sceGuTexEnvColor( out.TextureFactor.GetColour() ); break;
			}

			u32 tcc( GU_TCC_RGBA );
			switch( out.BlendAlphaMode )
			{
			case PBAM_RGBA:			tcc = GU_TCC_RGBA; break;
			case PBAM_RGB:			tcc = GU_TCC_RGB; break;
			}

			sceGuTexFunc( tfx, tcc );

			if( g_ROM.T1_HACK )
			{
				// NB if install_texture0 and install_texture1 are both set, 1 wins out
				texture_idx = install_texture1;

				if( install_texture1 & mTnL.Flags.Texture && (mTnL.Flags._u32 & (TNL_LIGHT|TNL_TEXGEN)) != (TNL_LIGHT|TNL_TEXGEN) )
				{
					v2 offset = -mTileTopLeft[ 1 ];
					v2 scale = mTileScale[ 1 ];
					sceGuTexOffset( offset.x * scale.x, offset.y * scale.y );
					sceGuTexScale( scale.x, scale.y );
				}
			}
			else
			{
				// NB if install_texture0 and install_texture1 are both set, 0 wins out
				texture_idx = install_texture0 ? 0 : 1;
			}

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
		if( !installed_texture ) sceGuDisable(GU_TEXTURE_2D);

		sceGuTexWrap( mTexWrap[texture_idx][0], mTexWrap[texture_idx][1] );

		sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );
	}
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
//*****************************************************************************
// Used for Blend Explorer, or Nasty texture
//*****************************************************************************
bool PSPRenderer::DebugBlendmode( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_flags, u64 mux )
{
	if( mNastyTexture && IsCombinerStateDisabled( mux ) )
	{
		// Use the nasty placeholder texture
		//
		sceGuEnable(GU_TEXTURE_2D);
		SelectPlaceholderTexture( PTT_SELECTED );
		sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
		sceGuTexMode(GU_PSM_8888,0,0,GL_TRUE);		// maxmips/a2/swizzle = 0
		sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );

		return true;

	}

	if(IsCombinerStateDisabled( mux ))
	{
		//Allow Blend Explorer
		//
		SBlendModeDetails		details;
		u32	num_cycles = gRDPOtherMode.cycle_type == CYCLE_2CYCLE ? 2 : 1;

		details.InstallTexture = true;
		details.EnvColour = mEnvColour;
		details.PrimColour = mPrimitiveColour;
		details.ColourAdjuster.Reset();

		//Insert the Blend Explorer
		BLEND_MODE_MAKER

		bool	installed_texture( false );

		if( details.InstallTexture )
		{
			if( mpTexture[ 0 ] != NULL )
			{
				const CRefPtr<CNativeTexture> texture( mpTexture[ 0 ]->GetTexture() );

				if(texture != NULL)
				{
					texture->InstallTexture();
					installed_texture = true;
				}
			}
		}

		// If no texture was specified, or if we couldn't load it, clear it out
		if( !installed_texture ) 
			sceGuDisable( GU_TEXTURE_2D );

		details.ColourAdjuster.Process( p_vertices, num_vertices );
		sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );

		return true;
	}

	return false;
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::DebugMux( const CBlendStates * states, DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_flags, u64 mux)
{
	bool	inexact( states->IsInexact() );

	// Only dump missing_mux when we awant to search for inexact blends aka HighlightInexactBlendModes is enabled.
	// Otherwise will dump lotsa of missing_mux even though is not needed since was handled correctly by auto blendmode thing - Salvy
	//
	if( inexact && gGlobalPreferences.HighlightInexactBlendModes)
	{
		if(mUnhandledCombinerStates.find( mux ) == mUnhandledCombinerStates.end())
		{
			char szFilePath[MAX_PATH+1];

			Dump_GetDumpDirectory(szFilePath, g_ROM.settings.GameName.c_str());

			IO::Path::Append(szFilePath, "missing_mux.txt");

			FILE * fh( fopen(szFilePath, mUnhandledCombinerStates.empty() ? "w" : "a") );
			if(fh != NULL)
			{
				PrintMux( fh, mux );
				fclose(fh);
			}

			mUnhandledCombinerStates.insert( mux );
		}
	}

	if(inexact && gGlobalPreferences.HighlightInexactBlendModes)
	{
		sceGuEnable( GU_TEXTURE_2D );
		sceGuTexMode( GU_PSM_8888, 0, 0, GL_TRUE );		// maxmips/a2/swizzle = 0

		// Use the nasty placeholder texture
		SelectPlaceholderTexture( PTT_MISSING );
		sceGuTexFunc( GU_TFX_REPLACE, GU_TCC_RGBA );
		sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );
	}
}

#endif	// DAEDALUS_DEBUG_DISPLAYLIST

extern void InitBlenderMode( u32 blender );
//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::RenderUsingCurrentBlendMode( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_mode, bool disable_zbuffer )
{

	static bool	ZFightingEnabled( false );

	DAEDALUS_PROFILE( "PSPRenderer::RenderUsingCurrentBlendMode" );

	if ( disable_zbuffer )
	{
		sceGuDisable(GU_DEPTH_TEST);
		sceGuDepthMask( GL_TRUE );	// GL_TRUE to disable z-writes
	}
	else
	{
		// Fixes Zfighting issues we have on the PSP.
		if( gRDPOtherMode.zmode == 3 )
		{
			if( !ZFightingEnabled )
			{
				ZFightingEnabled = true;						
				sceGuDepthRange(65535,80);
			}
		}
		else if( ZFightingEnabled )
		{
			ZFightingEnabled = false;						
			sceGuDepthRange(65535,0);
		}

		// Enable or Disable ZBuffer test
		if ( (mTnL.Flags.Zbuffer & gRDPOtherMode.z_cmp) | gRDPOtherMode.z_upd )
		{
			sceGuEnable(GU_DEPTH_TEST);
		}
		else
		{
			sceGuDisable(GU_DEPTH_TEST);
		}

		// GL_TRUE to disable z-writes
		sceGuDepthMask( gRDPOtherMode.z_upd ? GL_FALSE : GL_TRUE );
	}

	// Initiate Texture Filter
	//
	// G_TF_AVERAGE : 1, G_TF_BILERP : 2 (linear)
	// G_TF_POINT   : 0 (nearest)
	//
	if( (gRDPOtherMode.text_filt != G_TF_POINT) | (gGlobalPreferences.ForceLinearFilter) )
	{
		sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	}
	else
	{
		sceGuTexFilter(GU_NEAREST,GU_NEAREST);
	}

	u32 cycle_mode = gRDPOtherMode.cycle_type;

	// Initiate Blender
	//
	if(cycle_mode < CYCLE_COPY)
	{
		gRDPOtherMode.force_bl ? InitBlenderMode( gRDPOtherMode.blender ) : sceGuDisable( GU_BLEND );
	}

	// Initiate Alpha test
	//
	if( (gRDPOtherMode.alpha_compare == G_AC_THRESHOLD) && !gRDPOtherMode.alpha_cvg_sel )
	{
		// G_AC_THRESHOLD || G_AC_DITHER
		sceGuAlphaFunc( (mAlphaThreshold | g_ROM.ALPHA_HACK) ? GU_GEQUAL : GU_GREATER, mAlphaThreshold, 0xff);
		sceGuEnable(GU_ALPHA_TEST);
	}
	// I think this implies that alpha is coming from
	else if (gRDPOtherMode.cvg_x_alpha)
	{
		// Going over 0x70 brakes OOT, but going lesser than that makes lines on games visible...ex: Paper Mario.
		// ALso going over 0x30 breaks the birds in Tarzan :(. Need to find a better way to leverage this.
		sceGuAlphaFunc(GU_GREATER, 0x70, 0xff); 
		sceGuEnable(GU_ALPHA_TEST);
	}
	else
	{
		// Use CVG for pixel alpha
        sceGuDisable(GU_ALPHA_TEST);	
	}

	SBlendStateEntry		blend_entry;

	switch ( cycle_mode )
	{
		case CYCLE_COPY:		blend_entry.States = mCopyBlendStates; break;
		case CYCLE_FILL:		blend_entry.States = mFillBlendStates; break;
		case CYCLE_1CYCLE:		blend_entry = LookupBlendState( mMux, false ); break;
		case CYCLE_2CYCLE:		blend_entry = LookupBlendState( mMux, true ); break;
	}

	u32 render_flags( GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | render_mode );

	// Used for Blend Explorer, or Nasty texture
	//
#ifdef DAEDALUS_DEBUG_DISPLAYLIST	
	if( DebugBlendmode( p_vertices, num_vertices, triangle_mode, render_flags, mMux ) )	return;
#endif

	// This check is for inexact blends which were handled either by a custom blendmode or auto blendmode thing
	// 
	if( blend_entry.OverrideFunction != NULL )
	{
		// Used for dumping mux and highlight inexact blend
		//
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		DebugMux( blend_entry.States, p_vertices, num_vertices, triangle_mode, render_flags, mMux );
#endif

		// Local vars for now
		SBlendModeDetails details;

		details.EnvColour = mEnvColour;
		details.PrimColour = mPrimitiveColour;
		details.InstallTexture = true;
		details.ColourAdjuster.Reset();

		blend_entry.OverrideFunction( details );

		bool installed_texture( false );

		if( details.InstallTexture )
		{
			if( mpTexture[ g_ROM.T1_HACK ] != NULL )
			{
				const CRefPtr<CNativeTexture> texture( mpTexture[ g_ROM.T1_HACK ]->GetTexture() );

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

		sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );
	}
	else if( blend_entry.States != NULL )
	{
		RenderUsingRenderSettings( blend_entry.States, p_vertices, num_vertices, triangle_mode, render_flags );
	}
	else
	{
		// Set default states
		DAEDALUS_ERROR( "Unhandled blend mode" );
		sceGuDisable( GU_TEXTURE_2D );
		sceGuDrawArray( triangle_mode, render_flags, num_vertices, NULL, p_vertices );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::TexRect( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0, const v2 & uv1 )
{
	EnableTexturing( tile_idx );

	v2 screen0;
	v2 screen1;
	ConvertN64ToPsp( xy0, screen0 );
	ConvertN64ToPsp( xy1, screen1 );

	v2 tex_uv0;
	tex_uv0.x = uv0.x - mTileTopLeft[ 0 ].x;
	tex_uv0.y = uv0.y - mTileTopLeft[ 0 ].y;

	v2 tex_uv1;
	tex_uv1.x = uv1.x - mTileTopLeft[ 0 ].x;
	tex_uv1.y = uv1.y - mTileTopLeft[ 0 ].y;

	DL_PF( "    Screen:  %.1f,%.1f -> %.1f,%.1f", screen0.x, screen0.y, screen1.x, screen1.y );
	DL_PF( "    Texture: %.1f,%.1f -> %.1f,%.1f", tex_uv0.x, tex_uv0.y, tex_uv1.x, tex_uv1.y );

	const f32 depth = gRDPOtherMode.depth_source ? mPrimDepth : 0.0f;

#if 1	//1->SPRITE, 0->STRIP
	DaedalusVtx* p_vertices( (DaedalusVtx*)sceGuGetMemory(2 * sizeof(DaedalusVtx)) );

	p_vertices[0].Position.x = screen0.x;
	p_vertices[0].Position.y = screen0.y;
	p_vertices[0].Position.z = depth;
	p_vertices[0].Colour = c32(0xffffffff);
	p_vertices[0].Texture.x = tex_uv0.x;
	p_vertices[0].Texture.y = tex_uv0.y;

	p_vertices[1].Position.x = screen1.x;
	p_vertices[1].Position.y = screen1.y;
	p_vertices[1].Position.z = depth;
	p_vertices[1].Colour = c32(0xffffffff);
	p_vertices[1].Texture.x = tex_uv1.x;
	p_vertices[1].Texture.y = tex_uv1.y;

	RenderUsingCurrentBlendMode( p_vertices, 2, GU_SPRITES, GU_TRANSFORM_2D, gRDPOtherMode.depth_source ? false : true );
#else
	//	To be used with TRIANGLE_STRIP, which requires 40% less verts than TRIANGLE
	//	For reference for future ports and if SPRITES( which uses %60 less verts than TRIANGLE) causes issues
	DaedalusVtx* p_vertices( (DaedalusVtx*)sceGuGetMemory(4 * sizeof(DaedalusVtx)) );

	p_vertices[0].Position.x = screen0.x;
	p_vertices[0].Position.y = screen0.y;
	p_vertices[0].Position.z = depth;
	p_vertices[0].Colour = c32(0xffffffff);
	p_vertices[0].Texture.x = tex_uv0.x;
	p_vertices[0].Texture.y = tex_uv0.y;

	p_vertices[1].Position.x = screen1.x;
	p_vertices[1].Position.y = screen0.y;
	p_vertices[1].Position.z = depth;
	p_vertices[1].Colour = c32(0xffffffff);
	p_vertices[1].Texture.x = tex_uv1.x;
	p_vertices[1].Texture.y = tex_uv0.y;

	p_vertices[2].Position.x = screen0.x;
	p_vertices[2].Position.y = screen1.y;
	p_vertices[2].Position.z = depth;
	p_vertices[2].Colour = c32(0xffffffff);
	p_vertices[2].Texture.x = tex_uv0.x;
	p_vertices[2].Texture.y = tex_uv1.y;

	p_vertices[3].Position.x = screen1.x;
	p_vertices[3].Position.y = screen1.y;
	p_vertices[3].Position.z = depth;
	p_vertices[3].Colour = c32(0xffffffff);
	p_vertices[3].Texture.x = tex_uv1.x;
	p_vertices[3].Texture.y = tex_uv1.y;

	RenderUsingCurrentBlendMode( p_vertices, 4, GU_TRIANGLE_STRIP, GU_TRANSFORM_2D, gRDPOtherMode.depth_source ? false : true );
#endif

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	++m_dwNumRect;
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::TexRectFlip( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0, const v2 & uv1 )
{
	EnableTexturing( tile_idx );

	v2 screen0;
	v2 screen1;
	ConvertN64ToPsp( xy0, screen0 );
	ConvertN64ToPsp( xy1, screen1 );

	v2 tex_uv0;
	tex_uv0.x = uv0.x - mTileTopLeft[ 0 ].x;
	tex_uv0.y = uv0.y - mTileTopLeft[ 0 ].y;

	v2 tex_uv1;
	tex_uv1.x = uv1.x - mTileTopLeft[ 0 ].x;
	tex_uv1.y = uv1.y - mTileTopLeft[ 0 ].y;

	DL_PF( "    Screen:  %.1f,%.1f -> %.1f,%.1f", screen0.x, screen0.y, screen1.x, screen1.y );
	DL_PF( "    Texture: %.1f,%.1f -> %.1f,%.1f", tex_uv0.x, tex_uv0.y, tex_uv1.x, tex_uv1.y );

	DaedalusVtx* p_vertices( (DaedalusVtx*)sceGuGetMemory(4 * sizeof(DaedalusVtx)) );

	p_vertices[0].Position.x = screen0.x;
	p_vertices[0].Position.y = screen0.y;
	p_vertices[0].Position.z = 0.0f;
	p_vertices[0].Colour = c32(0xffffffff);
	p_vertices[0].Texture.x = tex_uv0.x;
	p_vertices[0].Texture.y = tex_uv0.y;

	p_vertices[1].Position.x = screen1.x;
	p_vertices[1].Position.y = screen0.y;
	p_vertices[1].Position.z = 0.0f;
	p_vertices[1].Colour = c32(0xffffffff);
	p_vertices[1].Texture.x = tex_uv0.x;
	p_vertices[1].Texture.y = tex_uv1.y;

	p_vertices[2].Position.x = screen0.x;
	p_vertices[2].Position.y = screen1.y;
	p_vertices[2].Position.z = 0.0f;
	p_vertices[2].Colour = c32(0xffffffff);
	p_vertices[2].Texture.x = tex_uv1.x;
	p_vertices[2].Texture.y = tex_uv0.y;

	p_vertices[3].Position.x = screen1.x;
	p_vertices[3].Position.y = screen1.y;
	p_vertices[3].Position.z = 0.0f;
	p_vertices[3].Colour = c32(0xffffffff);
	p_vertices[3].Texture.x = tex_uv1.x;
	p_vertices[3].Texture.y = tex_uv1.y;

	RenderUsingCurrentBlendMode( p_vertices, 4, GU_TRIANGLE_STRIP, GU_TRANSFORM_2D, true );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	++m_dwNumRect;
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::FillRect( const v2 & xy0, const v2 & xy1, u32 color )
{
/*
	if ( (gRDPOtherMode._u64 & 0xffff0000) == 0x5f500000 )	//Used by Wave Racer
	{
		// this blend mode is mem*0 + mem*1, so we don't need to render it... Very odd!
		DAEDALUS_ERROR("	mem*0 + mem*1 - skipped");
		return;
	}
*/
	// This if for C&C - It might break other stuff (I'm not sure if we should allow alpha or not..)
	//color |= 0xff000000;

	v2 screen0;
	v2 screen1;
	ConvertN64ToPsp( xy0, screen0 );
	ConvertN64ToPsp( xy1, screen1 );

	DL_PF( "    Screen:  %.1f,%.1f -> %.1f,%.1f", screen0.x, screen0.y, screen1.x, screen1.y );

	DaedalusVtx* p_vertices( (DaedalusVtx*)sceGuGetMemory(2 * sizeof(DaedalusVtx)) );

	// No need for Texture.x/y as we don't do any texturing for fillrect
	p_vertices[0].Position.x = screen0.x;
	p_vertices[0].Position.y = screen0.y;
	p_vertices[0].Position.z = 0.0f;
	p_vertices[0].Colour = c32(color);
	//p_vertices[0].Texture.x = 0.0f;
	//p_vertices[0].Texture.y = 0.0f;

	p_vertices[1].Position.x = screen1.x;
	p_vertices[1].Position.y = screen1.y;
	p_vertices[1].Position.z = 0.0f;
	p_vertices[1].Colour = c32(color);
	//p_vertices[1].Texture.x = 1.0f;
	//p_vertices[1].Texture.y = 0.0f;

	RenderUsingCurrentBlendMode( p_vertices, 2, GU_SPRITES, GU_TRANSFORM_2D, true );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	++m_dwNumRect;
#endif
}

//*****************************************************************************
// Returns true if triangle visible and rendered, false otherwise
//*****************************************************************************
bool PSPRenderer::AddTri(u32 v0, u32 v1, u32 v2)
{
	//DAEDALUS_PROFILE( "PSPRenderer::AddTri" );

	const u32 & f0( mVtxProjected[v0].ClipFlags );
	const u32 & f1( mVtxProjected[v1].ClipFlags );
	const u32 & f2( mVtxProjected[v2].ClipFlags );

	if ( f0 & f1 & f2 )
	{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		DL_PF("    Tri: %d,%d,%d (Culled -> NDC box)", v0, v1, v2);
		++m_dwNumTrisClipped;
#endif
		return false;
	}

	//
	//Cull BACK or FRONT faceing tris early in the pipeline //Corn
	//
	if( mTnL.Flags.TriCull )
	{
		const v4 & A( mVtxProjected[v0].ProjectedPos );
		const v4 & B( mVtxProjected[v1].ProjectedPos );
		const v4 & C( mVtxProjected[v2].ProjectedPos );

		//Avoid using 1/w, will use five more mults but save three divides //Corn
		//Precalc reused w combos so compiler does a proper job
		const f32 ABw(A.w*B.w);
		const f32 ACw(A.w*C.w);
		const f32 BCw(B.w*C.w);
		const f32 AxBC(A.x*BCw);
		const f32 AyBC(A.y*BCw);

		if( (((B.x*ACw - AxBC)*(C.y*ABw - AyBC) - (C.x*ABw - AxBC)*(B.y*ACw - AyBC)) * ABw * C.w) <= 0.f )
		{
			if( mTnL.Flags.CullBack )
			{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
				DL_PF("    Tri: %d,%d,%d (Culled -> Back Face)", v0, v1, v2);
				++m_dwNumTrisClipped;
#endif
				return false;
			}
		}
		else if( !mTnL.Flags.CullBack )
		{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			DL_PF("    Tri: %d,%d,%d (Culled -> Front Face)", v0, v1, v2);
			++m_dwNumTrisClipped;
#endif
			return false;
		}
	}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	DL_PF("    Tri: %d,%d,%d (Rendered)", v0, v1, v2);
	++m_dwNumTrisRendered;
#endif

	m_swIndexBuffer[ mNumIndices++ ] = (u16)v0;
	m_swIndexBuffer[ mNumIndices++ ] = (u16)v1;
	m_swIndexBuffer[ mNumIndices++ ] = (u16)v2;

	mVtxClipFlagsUnion |= f0 | f1 | f2;

	DAEDALUS_ASSERT( mNumIndices < MAX_VERTICES, " Array overflow, to many Indices" );

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::FlushTris()
{
	DAEDALUS_PROFILE( "PSPRenderer::FlushTris" );
	/*
	if ( mNumIndices == 0 )
	{
		DAEDALUS_ERROR("Call to FlushTris() with nothing to render");
		mVtxClipFlagsUnion = 0; // Reset software clipping detector
		return;
	}
	*/
	DAEDALUS_ASSERT( mNumIndices, "Call to FlushTris() with nothing to render" );

	u32				num_vertices;
	DaedalusVtx *	p_vertices;

	// If any bit is set here it means we have to clip the trianlges since PSP HW clipping sux!
	if(mVtxClipFlagsUnion != 0)
	{
		PrepareTrisClipped( &p_vertices, &num_vertices );
	}
	else
	{
		PrepareTrisUnclipped( &p_vertices, &num_vertices );
	}

	// No vertices to render? //Corn
	if( num_vertices == 0 )
	{
		mNumIndices = 0;
		mVtxClipFlagsUnion = 0;
		return;
	}

	// Hack for Pilotwings 64
	/*static bool skipNext=false;
	if( g_ROM.GameHacks == PILOT_WINGS )
	{
		if ( (g_DI.Address == g_CI.Address) && gRDPOtherMode.z_cmp+gRDPOtherMode.z_upd > 0 )
		{
			DAEDALUS_ERROR("Warning: using Flushtris to write Zbuffer" );
			mNumIndices = 0;
			mVtxClipFlagsUnion = 0;
			skipNext = true;
			return;
		}
		else if( skipNext )
		{
			skipNext = false;
			mNumIndices = 0;
			mVtxClipFlagsUnion = 0;
			return;
		}	
	}*/
	
	//
	// Process the software vertex buffer to apply a couple of
	// necessary changes to the texture coords (this is required
	// because some ucodes set the texture after setting the vertices)
	//
	if( mTnL.Flags.Texture )
	{
		EnableTexturing( mTextureTile );

		// Bias points in decal mode
		// Is this for Z-fight? not working to well for that, at least not on PSP//Corn
		//if(IsZModeDecal()) for( u32 v = 0; v < num_vertices; v++ ) p_vertices[v].Position.z += 3.14;

		if( (mTnL.Flags._u32 & (TNL_LIGHT|TNL_TEXGEN)) != (TNL_LIGHT|TNL_TEXGEN) )
		{
			sceGuTexOffset( -mTileTopLeft[ 0 ].x * mTileScale[ 0 ].x, -mTileTopLeft[ 0 ].y * mTileScale[ 0 ].y );
			sceGuTexScale( mTileScale[ 0 ].x, mTileScale[ 0 ].y );
		}
		else
		{
			sceGuTexOffset( 0.0f, 0.0f );
			sceGuTexScale( 1.0f, 1.0f );
		}
	}

	//
	//	Do BACK/FRONT culling in sceGE
	//	
	//if( mTnL.Flags.TriCull )
	//{
	//	sceGuFrontFace(mTnL.Flags.CullBack? GU_CCW : GU_CW);
	//	sceGuEnable(GU_CULL_FACE);
	//}
	//else
	//{
	//	sceGuDisable(GU_CULL_FACE);
	//}

	//
	// Check for depth source, this is for Nascar games, hopefully won't mess up anything
	DAEDALUS_ASSERT( !gRDPOtherMode.depth_source, " Warning : Using depth source in flushtris" );

	//
	//	Render out our vertices
	RenderUsingCurrentBlendMode( p_vertices, num_vertices, DRAW_MODE, GU_TRANSFORM_3D, gRDPOtherMode.depth_source ? true : false );

	//sceGuDisable(GU_CULL_FACE);

	mNumIndices = 0;
	mVtxClipFlagsUnion = 0;
}

//*****************************************************************************
//
//	The following clipping code was taken from The Irrlicht Engine.
//	See http://irrlicht.sourceforge.net/ for more information.
//	Copyright (C) 2002-2006 Nikolaus Gebhardt/Alten Thomas
//
//*****************************************************************************
const v4 __attribute__((aligned(16))) NDCPlane[6] =
{
	v4(  0.f,  0.f, -1.f, -1.f ),	// near
	v4(  0.f,  0.f,  1.f, -1.f ),	// far
	v4(  1.f,  0.f,  0.f, -1.f ),	// left
	v4( -1.f,  0.f,  0.f, -1.f ),	// right
	v4(  0.f,  1.f,  0.f, -1.f ),	// bottom
	v4(  0.f, -1.f,  0.f, -1.f )	// top
};

//
//Triangle clip using VFPU(fast)
//

#ifdef DAEDALUS_PSP_USE_VFPU
//*****************************************************************************
//VFPU tris clip
//*****************************************************************************
u32 clip_tri_to_frustum( DaedalusVtx4 * v0, DaedalusVtx4 * v1 )
{
	u32 vOut( 3 );

	vOut = _ClipToHyperPlane( v1, v0, &NDCPlane[0], vOut ); if( vOut < 3 ) return vOut;		// near
	vOut = _ClipToHyperPlane( v0, v1, &NDCPlane[1], vOut ); if( vOut < 3 ) return vOut;		// far
	vOut = _ClipToHyperPlane( v1, v0, &NDCPlane[2], vOut ); if( vOut < 3 ) return vOut;		// left
	vOut = _ClipToHyperPlane( v0, v1, &NDCPlane[3], vOut ); if( vOut < 3 ) return vOut;		// right
	vOut = _ClipToHyperPlane( v1, v0, &NDCPlane[4], vOut ); if( vOut < 3 ) return vOut;		// bottom
	vOut = _ClipToHyperPlane( v0, v1, &NDCPlane[5], vOut );									// top

	return vOut;
}

#else	// FPU/CPU(slower) 

//*****************************************************************************
//CPU interpolate line parameters
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
//CPU line clip to plane
//*****************************************************************************
static u32 clipToHyperPlane( DaedalusVtx4 * dest, const DaedalusVtx4 * source, u32 inCount, const v4 &plane )
{
	u32 outCount(0);
	DaedalusVtx4 * out(dest);

	const DaedalusVtx4 * a;
	const DaedalusVtx4 * b(source);

	f32 bDotPlane = b->ProjectedPos.Dot( plane );

	for( u32 i = 1; i < inCount + 1; ++i)
	{
		//a = &source[i%inCount];
		const s32 condition = i - inCount;
		const s32 index = (( ( condition >> 31 ) & ( i ^ condition ) ) ^ condition ); 
		a = &source[index];

		f32 aDotPlane = a->ProjectedPos.Dot( plane );

		// current point inside
		if ( aDotPlane <= 0.f )
		{
			// last point outside
			if ( bDotPlane > 0.f )
			{
				// intersect line segment with plane
				out->Interpolate( *b, *a, bDotPlane / (b->ProjectedPos - a->ProjectedPos).Dot( plane ) );
				out++;
				outCount++;
			}
			// copy current to out
			*out = *a;
			b = out;

			out++;
			outCount++;
		}
		else
		{
			// current point outside
			if ( bDotPlane <= 0.f )
			{
				// previous was inside, intersect line segment with plane
				out->Interpolate( *b, *a, bDotPlane / (b->ProjectedPos - a->ProjectedPos).Dot( plane ) );
				out++;
				outCount++;
			}
			b = a;
		}

		bDotPlane = aDotPlane;
	}

	return outCount;
}

//*****************************************************************************
//CPU tris clip to frustum
//*****************************************************************************
u32 clip_tri_to_frustum( DaedalusVtx4 * v0, DaedalusVtx4 * v1)
{
	u32 vOut(3);

	vOut = clipToHyperPlane( v1, v0, vOut, NDCPlane[0] ); if ( vOut < 3 ) return vOut;		// near
	vOut = clipToHyperPlane( v0, v1, vOut, NDCPlane[1] ); if ( vOut < 3 ) return vOut;		// far
	vOut = clipToHyperPlane( v1, v0, vOut, NDCPlane[2] ); if ( vOut < 3 ) return vOut;		// left
	vOut = clipToHyperPlane( v0, v1, vOut, NDCPlane[3] ); if ( vOut < 3 ) return vOut;		// right
	vOut = clipToHyperPlane( v1, v0, vOut, NDCPlane[4] ); if ( vOut < 3 ) return vOut;		// bottom
	vOut = clipToHyperPlane( v0, v1, vOut, NDCPlane[5] );									// top

	return vOut;
}
#endif	//CPU clip

//*****************************************************************************
//
//*****************************************************************************
namespace 
{
	DaedalusVtx4		temp_a[ 8 ];
	DaedalusVtx4		temp_b[ 8 ];
	// Flying Dragon clips more than 256
	const u32			MAX_CLIPPED_VERTS = 320;	
	DaedalusVtx			clip_vtx[MAX_CLIPPED_VERTS];
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
	//  Convert directly to PSP hardware format, that way we only copy 24 bytes instead of 64 bytes //Corn
	//
	u32 num_vertices = 0;

	for(u32 i = 0; i < (mNumIndices - 2);)
	{
		const u32 & idx0 = m_swIndexBuffer[ i++ ];
		const u32 & idx1 = m_swIndexBuffer[ i++ ];
		const u32 & idx2 = m_swIndexBuffer[ i++ ];

		//Check if any of the vertices are outside the clipbox (NDC), if so we need to clip the triangle
		if(mVtxProjected[idx0].ClipFlags | mVtxProjected[idx1].ClipFlags | mVtxProjected[idx2].ClipFlags)
		{
			temp_a[ 0 ] = mVtxProjected[ idx0 ];
			temp_a[ 1 ] = mVtxProjected[ idx1 ];
			temp_a[ 2 ] = mVtxProjected[ idx2 ];

			u32 out = clip_tri_to_frustum( temp_a, temp_b );
			//If we have less than 3 vertices left after the clipping
			//we can't make a triangle so we bail and skip rendering it.
			DL_PF("    Clip & re-tesselate [%d,%d,%d] with %d vertices", i-3, i-2, i-1, out);
			DL_PF("    %#5.3f, %#5.3f, %#5.3f", mVtxProjected[ idx0 ].ProjectedPos.x/mVtxProjected[ idx0 ].ProjectedPos.w, mVtxProjected[ idx0 ].ProjectedPos.y/mVtxProjected[ idx0 ].ProjectedPos.w, mVtxProjected[ idx0 ].ProjectedPos.z/mVtxProjected[ idx0 ].ProjectedPos.w);
			DL_PF("    %#5.3f, %#5.3f, %#5.3f", mVtxProjected[ idx1 ].ProjectedPos.x/mVtxProjected[ idx1 ].ProjectedPos.w, mVtxProjected[ idx1 ].ProjectedPos.y/mVtxProjected[ idx1 ].ProjectedPos.w, mVtxProjected[ idx1 ].ProjectedPos.z/mVtxProjected[ idx1 ].ProjectedPos.w);
			DL_PF("    %#5.3f, %#5.3f, %#5.3f", mVtxProjected[ idx2 ].ProjectedPos.x/mVtxProjected[ idx2 ].ProjectedPos.w, mVtxProjected[ idx2 ].ProjectedPos.y/mVtxProjected[ idx2 ].ProjectedPos.w, mVtxProjected[ idx2 ].ProjectedPos.z/mVtxProjected[ idx2 ].ProjectedPos.w);

			if( out < 3 )
				continue;

			// Retesselate
			u32 new_num_vertices( num_vertices + (out - 3) * 3 );
			if( new_num_vertices > MAX_CLIPPED_VERTS )
			{
				DAEDALUS_ERROR( "Too many clipped verts: %d", new_num_vertices );
				break;
			}
			//Make new triangles from the vertices we got back from clipping the original triangle
			for( u32 j = 0; j <= out - 3; ++j)
			{
#ifdef DAEDALUS_PSP_USE_VFPU
				_ConvertVertice( &clip_vtx[ num_vertices++ ], &temp_a[ 0 ]);
				_ConvertVertice( &clip_vtx[ num_vertices++ ], &temp_a[ j + 1 ]);
				_ConvertVertice( &clip_vtx[ num_vertices++ ], &temp_a[ j + 2 ]);
#else
				clip_vtx[ num_vertices ].Texture = temp_a[ 0 ].Texture; 	 
				clip_vtx[ num_vertices ].Colour = c32( temp_a[ 0 ].Colour ); 	 
				clip_vtx[ num_vertices ].Position.x = temp_a[ 0 ].TransformedPos.x;
				clip_vtx[ num_vertices ].Position.y = temp_a[ 0 ].TransformedPos.y;
				clip_vtx[ num_vertices++ ].Position.z = temp_a[ 0 ].TransformedPos.z;

				clip_vtx[ num_vertices ].Texture = temp_a[ j + 1 ].Texture; 	 
				clip_vtx[ num_vertices ].Colour = c32( temp_a[ j + 1 ].Colour ); 	 
				clip_vtx[ num_vertices ].Position.x = temp_a[ j + 1 ].TransformedPos.x;
				clip_vtx[ num_vertices ].Position.y = temp_a[ j + 1 ].TransformedPos.y;
				clip_vtx[ num_vertices++ ].Position.z = temp_a[ j + 1 ].TransformedPos.z;

				clip_vtx[ num_vertices ].Texture = temp_a[ j + 2 ].Texture; 	 
				clip_vtx[ num_vertices ].Colour = c32( temp_a[ j + 2 ].Colour ); 	 
				clip_vtx[ num_vertices ].Position.x = temp_a[ j + 2 ].TransformedPos.x;
				clip_vtx[ num_vertices ].Position.y = temp_a[ j + 2 ].TransformedPos.y;
				clip_vtx[ num_vertices++ ].Position.z = temp_a[ j + 2 ].TransformedPos.z;
#endif
			}
		}
		else	//Triangle is inside the clipbox so we just add it as it is.
		{
			if( num_vertices > (MAX_CLIPPED_VERTS - 3) )
			{
				DAEDALUS_ERROR( "Too many clipped verts: %d", num_vertices + 3 );
				break;
			}

#ifdef DAEDALUS_PSP_USE_VFPU
			_ConvertVertice( &clip_vtx[ num_vertices++ ], &mVtxProjected[ idx0 ]);
			_ConvertVertice( &clip_vtx[ num_vertices++ ], &mVtxProjected[ idx1 ]);
			_ConvertVertice( &clip_vtx[ num_vertices++ ], &mVtxProjected[ idx2 ]);
#else
			clip_vtx[ num_vertices ].Texture = mVtxProjected[ idx0 ].Texture; 	 
			clip_vtx[ num_vertices ].Colour = c32( mVtxProjected[ idx0 ].Colour ); 	 
			clip_vtx[ num_vertices ].Position.x = mVtxProjected[ idx0 ].TransformedPos.x;
			clip_vtx[ num_vertices ].Position.y = mVtxProjected[ idx0 ].TransformedPos.y;
			clip_vtx[ num_vertices++ ].Position.z = mVtxProjected[ idx0 ].TransformedPos.z;

			clip_vtx[ num_vertices ].Texture = mVtxProjected[ idx1 ].Texture; 	 
			clip_vtx[ num_vertices ].Colour = c32( mVtxProjected[ idx1 ].Colour ); 	 
			clip_vtx[ num_vertices ].Position.x = mVtxProjected[ idx1 ].TransformedPos.x;
			clip_vtx[ num_vertices ].Position.y = mVtxProjected[ idx1 ].TransformedPos.y;
			clip_vtx[ num_vertices++ ].Position.z = mVtxProjected[ idx1 ].TransformedPos.z;

			clip_vtx[ num_vertices ].Texture = mVtxProjected[ idx2 ].Texture; 	 
			clip_vtx[ num_vertices ].Colour = c32( mVtxProjected[ idx2 ].Colour ); 	 
			clip_vtx[ num_vertices ].Position.x = mVtxProjected[ idx2 ].TransformedPos.x;
			clip_vtx[ num_vertices ].Position.y = mVtxProjected[ idx2 ].TransformedPos.y;
			clip_vtx[ num_vertices++ ].Position.z = mVtxProjected[ idx2 ].TransformedPos.z;
#endif
		}
	}

	//
	//	Now the vertices have been clipped we need to write them into
	//	a buffer we obtain this from the display list.

	DaedalusVtx *p_vertices( (DaedalusVtx*)sceGuGetMemory(num_vertices * sizeof(DaedalusVtx)) );

	memcpy( p_vertices, clip_vtx, num_vertices * sizeof(DaedalusVtx) );

	*p_p_vertices = p_vertices;
	*p_num_vertices = num_vertices;
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::PrepareTrisUnclipped( DaedalusVtx ** p_p_vertices, u32 * p_num_vertices ) const
{
	DAEDALUS_PROFILE( "PSPRenderer::PrepareTrisUnclipped" );
	DAEDALUS_ASSERT( mNumIndices > 0, "The number of indices should have been checked" );

	u32				num_vertices( mNumIndices );
	DaedalusVtx *	p_vertices( (DaedalusVtx*)sceGuGetMemory(num_vertices*sizeof(DaedalusVtx)) );

	//
	//	Previously this code set up an index buffer to avoid processing the
	//	same vertices more than once - we avoid this now as there is apparently
	//	quite a large performance penalty associated with using these on the PSP.
	//
	//	http://forums.ps2dev.org/viewtopic.php?t=4703
	//
	//DAEDALUS_STATIC_ASSERT( MAX_CLIPPED_VERTS > ARRAYSIZE(m_swIndexBuffer) );

#ifdef DAEDALUS_PSP_USE_VFPU
	_ConvertVerticesIndexed( p_vertices, mVtxProjected, num_vertices, m_swIndexBuffer );
#else 	  
	 // 	 
	 //      Now we just shuffle all the data across directly (potentially duplicating verts) 	 
	 // 	 
	 for( u32 i = 0; i < mNumIndices; ++i ) 	 
	 { 	 
			 u32                     index( m_swIndexBuffer[ i ] ); 	 

			 p_vertices[ i ].Texture = mVtxProjected[ index ].Texture; 	 
			 p_vertices[ i ].Colour = c32( mVtxProjected[ index ].Colour ); 	 
			 p_vertices[ i ].Position.x = mVtxProjected[ index ].TransformedPos.x; 	 
			 p_vertices[ i ].Position.y = mVtxProjected[ index ].TransformedPos.y; 	 
			 p_vertices[ i ].Position.z = mVtxProjected[ index ].TransformedPos.z; 	 
	 } 	 
 #endif

	*p_p_vertices = p_vertices;
	*p_num_vertices = num_vertices;
}
//
//Transform using VFPU(fast)
//

#ifdef DAEDALUS_PSP_USE_VFPU
//*****************************************************************************
// Standard rendering pipeline using VFPU
//*****************************************************************************
void PSPRenderer::SetNewVertexInfo(u32 address, u32 v0, u32 n)
{
	const FiddledVtx * const pVtxBase( (const FiddledVtx*)(g_pu8RamBase + address) );
	const Matrix4x4 & matWorldProject( GetWorldProject() );

	//If WoldProjectmatrix has been modified due to insert or force matrix (Kirby, SSB / Tarzan, Rayman2, Donald duck, SW racer, Robot on wheels)
	//we need to update sceGU projmtx //Corn
	if( mWPmodified )
	{
		mWPmodified = false;
		mReloadProj = true;
		sceGuSetMatrix( GU_PROJECTION, reinterpret_cast< const ScePspFMatrix4 * >( &mWorldProject ) );
		mModelViewStack[mModelViewTop] = gMatrixIdentity;
	}

	const Matrix4x4 & matWorld( mModelViewStack[mModelViewTop] );

	DL_PF( "    Ambient color RGB[%f][%f][%f] Texture scale X[%f] Texture scale Y[%f]", mTnL.Lights[mTnL.NumLights].Colour.x, mTnL.Lights[mTnL.NumLights].Colour.y, mTnL.Lights[mTnL.NumLights].Colour.z, mTnL.TextureScaleX, mTnL.TextureScaleY);
	DL_PF( "    Light[%s] Texture[%s] EnvMap[%s] Fog[%s]", (mTnL.Flags.Light)? "On":"Off", (mTnL.Flags.Texture)? "On":"Off", (mTnL.Flags.TexGen)? (mTnL.Flags.TexGenLin)? "Linear":"Spherical":"Off", (mTnL.Flags.Fog)? "On":"Off");

	_TnLVFPU( &matWorld, &matWorldProject, pVtxBase, &mVtxProjected[v0], n, &mTnL );
}

//*****************************************************************************
//
//*****************************************************************************
/*void PSPRenderer::TestVFPUVerts( u32 v0, u32 num, const FiddledVtx * verts, const Matrix4x4 & mat_world )
{
	bool	env_map( (mTnL.Flags._u32 & (TNL_LIGHT|TNL_TEXGEN)) == (TNL_LIGHT|TNL_TEXGEN) );

	u32 vend( v0 + num );
	for (u32 i = v0; i < vend; i++)
	{
		const FiddledVtx & vert = verts[i - v0];
		const v4 &	projected( mVtxProjected[i].ProjectedPos );

		if (mTnL.Flags.Fog)
		{
			float eyespace_z = projected.z / projected.w;
			float fog_coeff = (eyespace_z * mTnL.FogMult) + mTnL.FogOffset;

			// Set the alpha
			f32 value = Clamp< f32 >( fog_coeff, 0.0f, 1.0f );

			if( pspFpuAbs( value - mVtxProjected[i].Colour.w ) > 0.01f )
			{
				printf( "Fog wrong: %f != %f\n", mVtxProjected[i].Colour.w, value );
			}
		}

		if (mTnL.Flags.Texture)
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
				tx = (float)vert.tu * mTnL.TextureScaleX;
				ty = (float)vert.tv * mTnL.TextureScaleY;
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
		//u32 flags = CalcClipFlags( projected );
		//if( flags != mVtxProjected[i].ClipFlags )
		//{
		//	printf( "flags wrong: %02x != %02x\n", mVtxProjected[i].ClipFlags, flags );
		//}
	}
}*/

//
//Transform using VFPU(fast) or FPU/CPU(slow)  
//

#else

//*****************************************************************************
//
//*****************************************************************************
v4 PSPRenderer::LightVert( const v3 & norm ) const
{
	
	u32 num = mTnL.NumLights;

	v4 result( mTnL.Lights[num].Colour.x, 
			   mTnL.Lights[num].Colour.y, 
			   mTnL.Lights[num].Colour.z,
			   mTnL.Lights[num].Colour.w ); // 1.0f


	for ( u32 l = 0; l < num; l++ )
	{
		f32 fCosT = norm.Dot( mTnL.Lights[l].Direction );
		if (fCosT > 0.0f)
		{
			result.x += mTnL.Lights[l].Colour.x * fCosT;
			result.y += mTnL.Lights[l].Colour.y * fCosT;
			result.z += mTnL.Lights[l].Colour.z * fCosT;
		}
	}

	//Clamp to 1.0
	if( result.x > 1.0f ) result.x = 1.0f;
	if( result.y > 1.0f ) result.y = 1.0f;
	if( result.z > 1.0f ) result.z = 1.0f;
	//result.w = 1.0f;

	return result;
}


//*****************************************************************************
// Standard rendering pipeline using FPU/CPU
//*****************************************************************************
void PSPRenderer::SetNewVertexInfo(u32 address, u32 v0, u32 n)
{
	const FiddledVtx * pVtxBase = (const FiddledVtx*)(g_pu8RamBase + address);
	const Matrix4x4 & matWorldProject( GetWorldProject() );

	//If WoldProjectmatrix has been modified due to insert or force matrix (Kirby, SSB / Tarzan, Rayman2, Donald duck, SW racer, Robot on wheels)
	//we need to update sceGU projmtx //Corn
	if( mWPmodified )
	{
		mWPmodified = false;
		mReloadProj = true;
		sceGuSetMatrix( GU_PROJECTION, reinterpret_cast< const ScePspFMatrix4 * >( &matWorldProject ) );
		mModelViewStack[mModelViewTop] = gMatrixIdentity;
	}

	const Matrix4x4 & matWorld( mModelViewStack[mModelViewTop] );

	DL_PF( "    Ambient color RGB[%f][%f][%f] Texture scale X[%f] Texture scale Y[%f]", mTnL.Lights[mTnL.NumLights].Colour.x, mTnL.Lights[mTnL.NumLights].Colour.y, mTnL.Lights[mTnL.NumLights].Colour.z, mTnL.TextureScaleX, mTnL.TextureScaleY);
	DL_PF( "    Light[%s] Texture[%s] EnvMap[%s] Fog[%s]", (mTnL.Flags.Light)? "On":"Off", (mTnL.Flags.Texture)? "On":"Off", (mTnL.Flags.TexGen)? (mTnL.Flags.TexGenLin)? "Linear":"Spherical":"Off", (mTnL.Flags.Fog)? "On":"Off");

	// Transform and Project + Lighting or Transform and Project with Colour
	//
	for (u32 i = v0; i < v0 + n; i++)
	{
		const FiddledVtx & vert = pVtxBase[i - v0];

		v4 w( f32( vert.x ), f32( vert.y ), f32( vert.z ), 1.0f );

		// VTX Transform
		//
		v4 & projected( mVtxProjected[i].ProjectedPos );
		projected = matWorldProject.Transform( w );
		mVtxProjected[i].TransformedPos = matWorld.Transform( w );

		//	Initialise the clipping flags
		//
		u32 clip_flags = 0;
		if		(projected.x < -projected.w)	clip_flags |= X_POS;
		else if (projected.x > projected.w)	clip_flags |= X_NEG;

		if		(projected.y < -projected.w)	clip_flags |= Y_POS;
		else if (projected.y > projected.w)	clip_flags |= Y_NEG;

		if		(projected.z < -projected.w)	clip_flags |= Z_POS;
		else if (projected.z > projected.w)	clip_flags |= Z_NEG;
		mVtxProjected[i].ClipFlags = clip_flags;

		// LIGHTING OR COLOR
		//
		if ( mTnL.Flags.Light )
		{
			v3	model_normal(f32( vert.norm_x ), f32( vert.norm_y ), f32( vert.norm_z ) );

			v3 vecTransformedNormal;
			vecTransformedNormal = matWorld.TransformNormal( model_normal );
			vecTransformedNormal.Normalise();

			mVtxProjected[i].Colour = LightVert(vecTransformedNormal);
			mVtxProjected[i].Colour.w = vert.rgba_a * (1.0f / 255.0f);

			// ENV MAPPING
			//
			if ( mTnL.Flags.TexGen )
			{
				// Update texture coords n.b. need to divide tu/tv by bogus scale on addition to buffer
				// If the vert is already lit, then there is no normal (and hence we can't generate tex coord)
#if 1			// 1->Lets use matWorldProject instead of mat_world for nicer effect (see SSV space ship) //Corn
				vecTransformedNormal = matWorldProject.TransformNormal( model_normal );
				vecTransformedNormal.Normalise();
#endif

				const v3 & norm = vecTransformedNormal;
				
				if( mTnL.Flags.TexGenLin )
				{
					mVtxProjected[i].Texture.x = 0.5f * ( 1.0f + norm.x );
					mVtxProjected[i].Texture.y = 0.5f * ( 1.0f + norm.y );
				}
				else
				{
					//Cheap way to do Acos(x)/Pi (abs() fixes star in SM64, sort of) //Corn
					f32 NormX = pspFpuAbs( norm.x );
					f32 NormY = pspFpuAbs( norm.y );
					mVtxProjected[i].Texture.x =  0.5f - 0.25f * NormX - 0.25f * NormX * NormX * NormX; 
					mVtxProjected[i].Texture.y =  0.5f - 0.25f * NormY - 0.25f * NormY * NormY * NormY;
				}
			}
			else
			{
				//Set Texture coordinates
				mVtxProjected[i].Texture.x = (float)vert.tu * mTnL.TextureScaleX;
				mVtxProjected[i].Texture.y = (float)vert.tv * mTnL.TextureScaleY;
			}
		}
		else
		{
			//if( mTnL.Flags.Shade )	//FLAT shade
			{	
				mVtxProjected[i].Colour = v4( vert.rgba_r * (1.0f / 255.0f), vert.rgba_g * (1.0f / 255.0f), vert.rgba_b * (1.0f / 255.0f), vert.rgba_a * (1.0f / 255.0f) );
			}
			/*else //Shade is disabled, doesn't work, is it even needed>?
			{	
				mVtxProjected[i].Colour = mPrimitiveColour.GetColourV4();
			}*/

			//Set Texture coordinates
			mVtxProjected[i].Texture.x = (float)vert.tu * mTnL.TextureScaleX;
			mVtxProjected[i].Texture.y = (float)vert.tv * mTnL.TextureScaleY;
		}

		/*
		// FOG
		//
		if ( mTnL.Flags.Fog )
		{
			float	fog_coeff;
			//if(fabsf(projected.w) > 0.0f)
			{
				float eyespace_z = projected.z / projected.w;
				fog_coeff = (eyespace_z * mTnL.FogMult) + mTnL.FogOffset;
			}
			//else
			//{
			//	fog_coeff = m_fFogOffset;
			//}

			// Set the alpha
			mVtxProjected[i].Colour.w = Clamp< f32 >( fog_coeff, 0.0f, 1.0f );
		}
		*/
	}
}

#endif	//Transform VFPU/FPU

#ifdef DAEDALUS_PSP_USE_VFPU
//*****************************************************************************
// Conker Bad Fur Day rendering pipeline
//*****************************************************************************
void PSPRenderer::SetNewVertexInfoConker(u32 address, u32 v0, u32 n)
{
	const FiddledVtx * const pVtxBase( (const FiddledVtx*)(g_pu8RamBase + address) );
	const Matrix4x4 & matProject( mProjectionStack[mProjectionTop] );
	const Matrix4x4 & matWorld( mModelViewStack[mModelViewTop] );

	DL_PF( "    Ambient color RGB[%f][%f][%f] Texture scale X[%f] Texture scale Y[%f]", mTnL.Lights[mTnL.NumLights].Colour.x, mTnL.Lights[mTnL.NumLights].Colour.y, mTnL.Lights[mTnL.NumLights].Colour.z, mTnL.TextureScaleX, mTnL.TextureScaleY);
	DL_PF( "    Light[%s] Texture[%s] EnvMap[%s] Fog[%s]", (mTnL.Flags.Light)? "On":"Off", (mTnL.Flags.Texture)? "On":"Off", (mTnL.Flags.TexGen)? (mTnL.Flags.TexGenLin)? "Linear":"Spherical":"Off", (mTnL.Flags.Fog)? "On":"Off");

	// Light is not handled for Conker
	//
	const s8 *mn = (s8*)(gAuxAddr);
	_TnLVFPUCBFD( &matWorld, &matProject, pVtxBase, &mVtxProjected[v0], n, &mTnL, mn, v0<<1 );
}

#else
//FPU/CPU version //Corn
//extern f32 gCoord_Mod[16];

void PSPRenderer::SetNewVertexInfoConker(u32 address, u32 v0, u32 n)
{
	//DBGConsole_Msg(0, "In SetNewVertexInfo");
	const FiddledVtx * const pVtxBase( (const FiddledVtx*)(g_pu8RamBase + address) );
	const Matrix4x4 & matProject( mProjectionStack[mProjectionTop] );
	const Matrix4x4 & matWorld( mModelViewStack[mModelViewTop] );

	DL_PF( "    Ambient color RGB[%f][%f][%f] Texture scale X[%f] Texture scale Y[%f]", mTnL.Lights[mTnL.NumLights].Colour.x, mTnL.Lights[mTnL.NumLights].Colour.y, mTnL.Lights[mTnL.NumLights].Colour.z, mTnL.TextureScaleX, mTnL.TextureScaleY);
	DL_PF( "    Light[%s] Texture[%s] EnvMap[%s] Fog[%s]", (mTnL.Flags.Light)? "On":"Off", (mTnL.Flags.Texture)? "On":"Off", (mTnL.Flags.TexGen)? (mTnL.Flags.TexGenLin)? "Linear":"Spherical":"Off", (mTnL.Flags.Fog)? "On":"Off");

	//Model normal base vector
	const s8 *mn = (s8*)(gAuxAddr);

	// Transform and Project + Lighting or Transform and Project with Colour
	//
	for (u32 i = v0; i < v0 + n; i++)
	{
		const FiddledVtx & vert = pVtxBase[i - v0];

		v4 w( f32( vert.x ), f32( vert.y ), f32( vert.z ), 1.0f );

		// VTX Transform
		//
		v4 & transformed( mVtxProjected[i].TransformedPos );
		transformed = matWorld.Transform( w );
		v4 & projected( mVtxProjected[i].ProjectedPos );
		projected = matProject.Transform( transformed );

		//	Initialise the clipping flags
		//
		u32 clip_flags = 0;
		if		(projected.x < -projected.w)	clip_flags |= X_POS;
		else if (projected.x > projected.w)	clip_flags |= X_NEG;

		if		(projected.y < -projected.w)	clip_flags |= Y_POS;
		else if (projected.y > projected.w)	clip_flags |= Y_NEG;

		if		(projected.z < -projected.w)	clip_flags |= Z_POS;
		else if (projected.z > projected.w)	clip_flags |= Z_NEG;
		mVtxProjected[i].ClipFlags = clip_flags;

		// LIGHTING OR COLOR
		//
		if ( mTnL.Flags.Light )
		{
			v4	result( mTnL.Lights[mTnL.NumLights].Colour );

			for ( u32 k = 1; k < mTnL.NumLights; k++ )
			{
				result.x += mTnL.Lights[k].Colour.x;
				result.y += mTnL.Lights[k].Colour.y;
				result.z += mTnL.Lights[k].Colour.z;
			}

			//Clamp to 1.0
			if( result.x > 1.0f ) result.x = 1.0f;
			if( result.y > 1.0f ) result.y = 1.0f;
			if( result.z > 1.0f ) result.z = 1.0f;

			result.x *= (f32)vert.rgba_r * (1.0f / 255.0f);
			result.y *= (f32)vert.rgba_g * (1.0f / 255.0f);
			result.z *= (f32)vert.rgba_b * (1.0f / 255.0f);
			result.w  = (f32)vert.rgba_a * (1.0f / 255.0f);

			mVtxProjected[i].Colour = result;

			// ENV MAPPING
			//
			if ( mTnL.Flags.TexGen )
			{
				v3 model_normal( mn[((i<<1)+0)^3] , mn[((i<<1)+1)^3], vert.normz );
				v3 vecTransformedNormal = matWorld.TransformNormal( model_normal );
				vecTransformedNormal.Normalise();

				const v3 & norm = vecTransformedNormal;
				
				if( mTnL.Flags.TexGenLin )
				{
					//Cheap way to do Acos(x)/Pi //Corn
					mVtxProjected[i].Texture.x =  0.5f - 0.25f * norm.x - 0.25f * norm.x * norm.x * norm.x; 
					mVtxProjected[i].Texture.y =  0.5f - 0.25f * norm.y - 0.25f * norm.y * norm.y * norm.y;
				}
				else
				{
					mVtxProjected[i].Texture.x = 0.5f * ( 1.0f + norm.x );
					mVtxProjected[i].Texture.y = 0.5f * ( 1.0f + norm.y );
				}
			}
			else
			{	//TEXTURE
				mVtxProjected[i].Texture.x = (float)vert.tu * mTnL.TextureScaleX;
				mVtxProjected[i].Texture.y = (float)vert.tv * mTnL.TextureScaleY;
			}
		}
		else
		{
			if( mTnL.Flags.Shade )
			{	//FLAT shade
				mVtxProjected[i].Colour.x = (f32)vert.rgba_r * (1.0f / 255.0f);
				mVtxProjected[i].Colour.y = (f32)vert.rgba_g * (1.0f / 255.0f);
				mVtxProjected[i].Colour.z = (f32)vert.rgba_b * (1.0f / 255.0f);
				mVtxProjected[i].Colour.w = (f32)vert.rgba_a * (1.0f / 255.0f);
			}
			else
			{	//Shade is disabled
				mVtxProjected[i].Colour = mPrimitiveColour.GetColourV4();
			}

			//TEXTURE
			mVtxProjected[i].Texture.x = (float)vert.tu * mTnL.TextureScaleX;
			mVtxProjected[i].Texture.y = (float)vert.tv * mTnL.TextureScaleY;
		}
	}
}
#endif

//*****************************************************************************
// Assumes address has already been checked!
// DKR/Jet Force Gemini rendering pipeline
//*****************************************************************************
extern u32 gDKRCMatrixIndex;
extern u32 gDKRVtxCount;
extern bool gDKRBillBoard;

void PSPRenderer::SetNewVertexInfoDKR(u32 address, u32 v0, u32 n)
{
	gDKRVtxCount += n;

	u32 pVtxBase = u32(g_pu8RamBase + address);
	const Matrix4x4 & matWorldProject( mProjectionStack[gDKRCMatrixIndex] );

	DL_PF( "    Ambient color RGB[%f][%f][%f] Texture scale X[%f] Texture scale Y[%f]", mTnL.Lights[mTnL.NumLights].Colour.x, mTnL.Lights[mTnL.NumLights].Colour.y, mTnL.Lights[mTnL.NumLights].Colour.z, mTnL.TextureScaleX, mTnL.TextureScaleY);
	DL_PF( "    Light[%s] Texture[%s] EnvMap[%s] Fog[%s]", (mTnL.Flags.Light)? "On":"Off", (mTnL.Flags.Texture)? "On":"Off", (mTnL.Flags.TexGen)? (mTnL.Flags.TexGenLin)? "Linear":"Spherical":"Off", (mTnL.Flags.Fog)? "On":"Off");
	DL_PF( "    CMtx[%d] Add base[%s]", gDKRCMatrixIndex, gDKRBillBoard? "On":"Off");

	if( gDKRBillBoard )
	{	//Copy vertices adding base vector and the color data
		mWPmodified = false;

#ifdef DAEDALUS_PSP_USE_VFPU
		_TnLVFPUDKRB( n, &mProjectionStack[0], (const FiddledVtx*)pVtxBase, &mVtxProjected[v0] );
#else
		v4 & BaseVec( mVtxProjected[0].TransformedPos );
	
		//Hack to worldproj matrix to scale and rotate billbords //Corn
		Matrix4x4 mat( mProjectionStack[0]);
		mat.mRaw[0] *= mProjectionStack[2].mRaw[0] * 0.5f;
		mat.mRaw[4] *= mProjectionStack[2].mRaw[0] * 0.5f;
		mat.mRaw[8] *= mProjectionStack[2].mRaw[0] * 0.5f;
		mat.mRaw[1] *= mProjectionStack[2].mRaw[0] * 0.375f;
		mat.mRaw[5] *= mProjectionStack[2].mRaw[0] * 0.375f;
		mat.mRaw[9] *= mProjectionStack[2].mRaw[0] * 0.375f;
		mat.mRaw[2] *= mProjectionStack[2].mRaw[10] * 0.5f;
		mat.mRaw[6] *= mProjectionStack[2].mRaw[10] * 0.5f;
		mat.mRaw[10] *= mProjectionStack[2].mRaw[10] * 0.5f;

		for (u32 i = v0; i < v0 + n; i++)
		{
			v3 w;
			w.x = *(s16*)((pVtxBase + 0) ^ 2);
			w.y = *(s16*)((pVtxBase + 2) ^ 2);
			w.z = *(s16*)((pVtxBase + 4) ^ 2);

			w = mat.TransformNormal( w );

			v4 & transformed( mVtxProjected[i].TransformedPos );
			transformed.x = BaseVec.x + w.x;
			transformed.y = BaseVec.y + w.y;
			transformed.z = BaseVec.z + w.z;
			transformed.w = 1.0f;

			// Set Clipflags, zero clippflags if billbording //Corn
			mVtxProjected[i].ClipFlags = 0;

			// Assign true vert colour
			const u32 WL = *(u16*)((pVtxBase + 6) ^ 2);
			const u32 WH = *(u16*)((pVtxBase + 8) ^ 2);

			mVtxProjected[i].Colour.x = (1.0f / 255.0f) * (WL >> 8);
			mVtxProjected[i].Colour.y = (1.0f / 255.0f) * (WL & 0xFF);
			mVtxProjected[i].Colour.z = (1.0f / 255.0f) * (WH >> 8);
			mVtxProjected[i].Colour.w = (1.0f / 255.0f) * (WH & 0xFF);

			pVtxBase += 10;
		}
#endif
	}
	else
	{	//Normal path for transform of triangles
		if( mWPmodified )
		{	//Only reload matrix if it has been changed and no billbording //Corn
			mWPmodified = false;
			sceGuSetMatrix( GU_PROJECTION, reinterpret_cast< const ScePspFMatrix4 * >( &matWorldProject) );
		}
#ifdef DAEDALUS_PSP_USE_VFPU
		_TnLVFPUDKR( n, &matWorldProject, (const FiddledVtx*)pVtxBase, &mVtxProjected[v0] );
#else
		for (u32 i = v0; i < v0 + n; i++)
		{
			v4 & transformed( mVtxProjected[i].TransformedPos );
			transformed.x = *(s16*)((pVtxBase + 0) ^ 2);
			transformed.y = *(s16*)((pVtxBase + 2) ^ 2);
			transformed.z = *(s16*)((pVtxBase + 4) ^ 2);
			transformed.w = 1.0f;

			v4 & projected( mVtxProjected[i].ProjectedPos );
			projected = matWorldProject.Transform( transformed );	//Do projection

			// Set Clipflags
			u32 clip_flags = 0;
			if		(projected.x < -projected.w)	clip_flags |= X_POS;
			else if (projected.x > projected.w)	clip_flags |= X_NEG;

			if		(projected.y < -projected.w)	clip_flags |= Y_POS;
			else if (projected.y > projected.w)	clip_flags |= Y_NEG;

			if		(projected.z < -projected.w)	clip_flags |= Z_POS;
			else if (projected.z > projected.w)	clip_flags |= Z_NEG;
			mVtxProjected[i].ClipFlags = clip_flags;

			// Assign true vert colour
			const u32 WL = *(u16*)((pVtxBase + 6) ^ 2);
			const u32 WH = *(u16*)((pVtxBase + 8) ^ 2);

			mVtxProjected[i].Colour.x = (1.0f / 255.0f) * (WL >> 8);
			mVtxProjected[i].Colour.y = (1.0f / 255.0f) * (WL & 0xFF);
			mVtxProjected[i].Colour.z = (1.0f / 255.0f) * (WH >> 8);
			mVtxProjected[i].Colour.w = (1.0f / 255.0f) * (WH & 0xFF);

			pVtxBase += 10;
		}
#endif
	}
}

//*****************************************************************************
// Perfect Dark rendering pipeline
//*****************************************************************************
#ifdef DAEDALUS_PSP_USE_VFPU
void PSPRenderer::SetNewVertexInfoPD(u32 address, u32 v0, u32 n)
{
	const FiddledVtxPD * const pVtxBase = (const FiddledVtxPD*)(g_pu8RamBase + address);

	const Matrix4x4 & matWorld( mModelViewStack[mModelViewTop] );
	const Matrix4x4 & matProject( mProjectionStack[mProjectionTop] );

	DL_PF( "    Ambient color RGB[%f][%f][%f] Texture scale X[%f] Texture scale Y[%f]", mTnL.Lights[mTnL.NumLights].Colour.x, mTnL.Lights[mTnL.NumLights].Colour.y, mTnL.Lights[mTnL.NumLights].Colour.z, mTnL.TextureScaleX, mTnL.TextureScaleY);
	DL_PF( "    Light[%s] Texture[%s] EnvMap[%s] Fog[%s]", (mTnL.Flags.Light)? "On":"Off", (mTnL.Flags.Texture)? "On":"Off", (mTnL.Flags.TexGen)? (mTnL.Flags.TexGenLin)? "Linear":"Spherical":"Off", (mTnL.Flags.Fog)? "On":"Off");

	//Model & Color base vector
	const u8 *mn = (u8*)gAuxAddr;

	_TnLVFPUPD( &matWorld, &matProject, pVtxBase, &mVtxProjected[v0], n, &mTnL, mn );
}

#else
void PSPRenderer::SetNewVertexInfoPD(u32 address, u32 v0, u32 n)
{
	const FiddledVtxPD * const pVtxBase = (const FiddledVtxPD*)(g_pu8RamBase + address);

	const Matrix4x4 & matWorld( mModelViewStack[mModelViewTop] );
	const Matrix4x4 & matProject( mProjectionStack[mProjectionTop] );

	DL_PF( "    Ambient color RGB[%f][%f][%f] Texture scale X[%f] Texture scale Y[%f]", mTnL.Lights[mTnL.NumLights].Colour.x, mTnL.Lights[mTnL.NumLights].Colour.y, mTnL.Lights[mTnL.NumLights].Colour.z, mTnL.TextureScaleX, mTnL.TextureScaleY);
	DL_PF( "    Light[%s] Texture[%s] EnvMap[%s] Fog[%s]", (mTnL.Flags.Light)? "On":"Off", (mTnL.Flags.Texture)? "On":"Off", (mTnL.Flags.TexGen)? (mTnL.Flags.TexGenLin)? "Linear":"Spherical":"Off", (mTnL.Flags.Fog)? "On":"Off");

	//Model normal base vector
	const s8 *mn  = (s8*)(gAuxAddr);
	//Color base vector
	const u8 *col = (u8*)(gAuxAddr);

	for (u32 i = v0; i < v0 + n; i++)
	{
		const FiddledVtxPD & vert = pVtxBase[i - v0];

		v4 w( f32( vert.x ), f32( vert.y ), f32( vert.z ), 1.0f );

		// VTX Transform
		//
		v4 & transformed( mVtxProjected[i].TransformedPos );
		transformed = matWorld.Transform( w );
		v4 & projected( mVtxProjected[i].ProjectedPos );
		projected = matProject.Transform( transformed );


		// Set Clipflags //Corn
		u32 clip_flags = 0;
		if		(projected.x < -projected.w)	clip_flags |= X_POS;
		else if (projected.x > projected.w)	clip_flags |= X_NEG;

		if		(projected.y < -projected.w)	clip_flags |= Y_POS;
		else if (projected.y > projected.w)	clip_flags |= Y_NEG;

		if		(projected.z < -projected.w)	clip_flags |= Z_POS;
		else if (projected.z > projected.w)	clip_flags |= Z_NEG;
		mVtxProjected[i].ClipFlags = clip_flags;

		if( mTnL.Flags.Light )
		{
			v3	model_normal((f32)mn[vert.cidx+3], (f32)mn[vert.cidx+2], (f32)mn[vert.cidx+1] );

			v3 vecTransformedNormal;
			vecTransformedNormal = matWorld.TransformNormal( model_normal );
			vecTransformedNormal.Normalise();

			mVtxProjected[i].Colour = LightVert(vecTransformedNormal);
			mVtxProjected[i].Colour.w = (f32)col[vert.cidx+0] * (1.0f / 255.0f);

			if ( mTnL.Flags.TexGen )
			{
				const v3 & norm = vecTransformedNormal;

				//Env mapping
				if( mTnL.Flags.TexGenLin )
				{	//Cheap way to do Acos(x)/Pi //Corn
					mVtxProjected[i].Texture.x =  0.5f - 0.25f * norm.x - 0.25f * norm.x * norm.x * norm.x;
					mVtxProjected[i].Texture.y =  0.5f - 0.25f * norm.y - 0.25f * norm.y * norm.y * norm.y;
				}
				else
				{
					mVtxProjected[i].Texture.x = 0.5f * ( 1.0f + norm.x );
					mVtxProjected[i].Texture.y = 0.5f * ( 1.0f + norm.y );
				}
			}
			else
			{
				mVtxProjected[i].Texture.x = (float)vert.tu * mTnL.TextureScaleX;
				mVtxProjected[i].Texture.y = (float)vert.tv * mTnL.TextureScaleY;
			}
		}
		else
		{
			if( mTnL.Flags.Shade )
			{	//FLAT shade
				mVtxProjected[i].Colour.x = (f32)col[vert.cidx+3] * (1.0f / 255.0f);
				mVtxProjected[i].Colour.y = (f32)col[vert.cidx+2] * (1.0f / 255.0f);
				mVtxProjected[i].Colour.z = (f32)col[vert.cidx+1] * (1.0f / 255.0f);
				mVtxProjected[i].Colour.w = (f32)col[vert.cidx+0] * (1.0f / 255.0f);
			}
			else
			{	//Shade is disabled
				mVtxProjected[i].Colour = mPrimitiveColour.GetColourV4();
			}

			mVtxProjected[i].Texture.x = (float)vert.tu * mTnL.TextureScaleX;
			mVtxProjected[i].Texture.y = (float)vert.tv * mTnL.TextureScaleY;
		}
	}
}
#endif

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::ModifyVertexInfo(u32 whered, u32 vert, u32 val)
{
	switch ( whered )
	{
		case G_MWO_POINT_RGBA:
			{
				DL_PF("    Setting RGBA to 0x%08x", val);
				SetVtxColor( vert, c32( val ) );
			}
			break;

		case G_MWO_POINT_ST:
			{
				s16 tu = s16(val >> 16);
				s16 tv = s16(val & 0xFFFF);
				DL_PF( "    Setting tu/tv to %f, %f", tu/32.0f, tv/32.0f );
				SetVtxTextureCoord( vert, tu, tv );
			}
			break;

		case G_MWO_POINT_XYSCREEN:
			{
				if( g_ROM.GameHacks == TARZAN ) return;

				s16 x = (u16)(val >> 16) >> 2;
				s16 y = (u16)(val & 0xFFFF) >> 2;

				// Fixes the blocks lining up backwards in New Tetris
				//
				x -= uViWidth / 2;
				y = uViHeight / 2 - y;

				DL_PF("    Modify vert %d: x=%d, y=%d", vert, x, y);
				
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
				//s32 z = val >> 16;
				//DL_PF( "      Setting ZScreen to 0x%08x", z );
				DL_PF( "    Setting ZScreen");
				//Not sure about the scaling here //Corn
				//SetVtxZ( vert, (( (f32)z / 0x03FF ) + 0.5f ) / 2.0f );
				//SetVtxZ( vert, (( (f32)z ) + 0.5f ) / 2.0f );
			}
			break;

		default:
			DBGConsole_Msg( 0, "ModifyVtx - Setting vert data 0x%02x, 0x%08x", whered, val );
			DL_PF( "    Setting unknown value: 0x%02x, 0x%08x", whered, val );
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
/*
inline void PSPRenderer::SetVtxZ( u32 vert, float z )
{
	DAEDALUS_ASSERT( vert < MAX_VERTS, " SetVtxZ : Reached max of verts");

	mVtxProjected[vert].TransformedPos.z = z;
}
*/
//*****************************************************************************
//
//*****************************************************************************
inline void PSPRenderer::SetVtxXY( u32 vert, float x, float y )
{
	DAEDALUS_ASSERT( vert < MAX_VERTS, " SetVtxXY : Reached max of verts %d",vert);

	mVtxProjected[vert].TransformedPos.x = x;
	mVtxProjected[vert].TransformedPos.y = y;
}

//*****************************************************************************
// Init matrix stack to identity matrices (called once per frame)
//*****************************************************************************
void PSPRenderer::ResetMatrices()
{
	mProjectionTop = mModelViewTop = 0;
	mProjectionStack[0] = mModelViewStack[0] = gMatrixIdentity;
	mWorldProjectValid = false;
}

//*****************************************************************************
//
//*****************************************************************************
inline void	PSPRenderer::EnableTexturing( u32 tile_idx )
{
	EnableTexturing( 0, tile_idx );

//#ifdef RDP_USE_TEXEL1

	if ( g_ROM.T1_HACK & !gRDPOtherMode.text_lod )
	{
		// LOD is disabled - use two textures
		EnableTexturing( 1, tile_idx + 1 );
	}
	//else
	//{
	//	// LOD is enabled - use the highest detail texture in texel1
	//	EnableTexturing( 1, tile_idx );
	//}
//#endif
}

//*****************************************************************************
//
//*****************************************************************************
void	PSPRenderer::EnableTexturing( u32 index, u32 tile_idx )
{
	DAEDALUS_PROFILE( "PSPRenderer::EnableTexturing" );

	DAEDALUS_ASSERT( tile_idx < 8, "Invalid tile index %d", tile_idx );
	DAEDALUS_ASSERT( index < NUM_N64_TEXTURES, "Invalid texture index %d", index );

	const TextureInfo & ti( gRDPStateManager.GetTextureDescriptor( tile_idx ) );

#ifndef DAEDALUS_DEBUG_DISPLAYLIST
	// Avoid texture update, if texture is the same as last time around.
	if( (mpTexture[ index ] != NULL) && (mpTexture[ index ]->GetTextureInfo() == ti) ) return;
#endif

	//	Initialise the wrapping/texture offset first, which can be set
	//	independently of the actual texture.
	//
	const RDP_Tile & rdp_tile( gRDPStateManager.GetTile( tile_idx ) );
	const RDP_TileSize & tile_size( gRDPStateManager.GetTileSize( tile_idx ) );

	// Initialise the clamping state. When the mask is 0, it forces clamp mode.
	//
	u32 mode_u = (rdp_tile.clamp_s | ( rdp_tile.mask_s == 0)) ? GU_CLAMP : GU_REPEAT;
	u32 mode_v = (rdp_tile.clamp_t | ( rdp_tile.mask_t == 0)) ? GU_CLAMP : GU_REPEAT;

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
	if( tile_size.GetWidth() > ti.GetWidth() )
	{
		// This breaks the Sun, and other textures in Zelda. Breaks Mario's hat in SSB, and other textures, and foes in Kirby 64's cutscenes
		// ToDo : Find a proper workaround for this, if this disabled the castle in Link's stage in SSB is broken :/
		// Do a hack just for Zelda for now..
		//
		if( g_ROM.ZELDA_HACK )
			 mode_u = GU_CLAMP;
		else
			mode_u = GU_REPEAT; 
	}
	if( tile_size.GetHeight() > ti.GetHeight() ) mode_v = GU_REPEAT;

	mTexWrap[ index ][ 0 ] = mode_u;
	mTexWrap[ index ][ 1 ] = mode_v;

	sceGuTexWrap( mode_u, mode_v );

	// XXXX Double check this
	mTileTopLeft[ index ].x = f32(tile_size.left) / 4.0f;
	mTileTopLeft[ index ].y = f32(tile_size.top) / 4.0f;

	DL_PF( "    Use Tile[%d] as Texture[%d] [%dx%d] [%s/%dbpp] [%s u, %s v] -> Adr[0x%08x] PAL[0x%x] Hash[0x%08x] Pitch[%d] TopLeft[%0.3f|%0.3f] Scale[%0.3f|%0.3f]",
			tile_idx, index, ti.GetWidth(), ti.GetHeight(), ti.GetFormatName(), ti.GetSizeInBits(),
			(mode_u==GU_CLAMP)? "Clamp" : "Repeat", (mode_v==GU_CLAMP)? "Clamp" : "Repeat",
			ti.GetLoadAddress(), (u32)ti.GetPalettePtr(), ti.GetHashCode(), ti.GetPitch(),
			mTileTopLeft[ index ].x, mTileTopLeft[ index ].y, mTileScale[ index ].x, mTileScale[ index ].y );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	// Avoid texture update, if texture is the same as last time around.
	if( (mpTexture[ index ] != NULL) && (mpTexture[ index ]->GetTextureInfo() == ti) ) return;
#endif

	// Check for 0 width/height textures
	if( (ti.GetWidth() == 0) || (ti.GetHeight() == 0) )
	{
		DAEDALUS_DL_ERROR( "Loading texture with 0 width/height" );
	}
	else
	{
		const CRefPtr<CTexture>	texture( CTextureCache::Get()->GetTexture( &ti ) );

		if( texture != NULL )
		{
			//
			//	Avoid update check and divides if the texture is already installed
			//
			if( texture != mpTexture[ index ] )
			{
				texture->UpdateIfNecessary();

				mpTexture[ index ] = texture;

				const CRefPtr<CNativeTexture> & native_texture( texture->GetTexture() );

				//If second texture is loaded try to merge two textures RGB(T0) + A(T1) into one RGBA(T1) //Corn
				//If T1 Hack is not enabled index can never be other than 0
				if(index) 
				{
					const TextureInfo & ti0(mpTexture[ 0 ]->GetTextureInfo());

					if((ti0.GetFormat() == 0) && (ti.GetFormat() == 4) && (ti.GetWidth() == ti0.GetWidth()) && (ti.GetHeight() == ti0.GetHeight()))
					{
						const CRefPtr<CNativeTexture> & native_texture0( mpTexture[ 0 ]->GetTexture() );
						u32* dst=(u32*)(native_texture->GetData());
						u32* src=(u32*)(native_texture0->GetData());
						
						//We do two pixels in one go since its 16bit (RGBA_4444) //Corn
						u32 size = native_texture->GetWidth() * native_texture->GetHeight() >> 1;
						for(u32 i=0; i < size ; i++)
						{
							*dst = (*dst & 0xF000F000) | (*src & 0x0FFF0FFF);
							dst++;
							src++;
						}
					}
				}

				if( native_texture != NULL )
				{
					mTileScale[ index ].x = native_texture->GetScaleX();
					mTileScale[ index ].y = native_texture->GetScaleY();
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
	if( x1 > uViWidth )  x1 = uViWidth;
	if( y1 > uViHeight ) y1 = uViHeight;

	v2 n64_coords_tl( x0, y0 );
	v2 n64_coords_br( x1, y1 );

	v2 psp_coords_tl;
	v2 psp_coords_br;
	ConvertN64ToPsp( n64_coords_tl, psp_coords_tl );
	ConvertN64ToPsp( n64_coords_br, psp_coords_br );

	// N.B. Think the arguments are x0,y0,x1,y1, and not x,y,w,h as the docs describe
	//Clamp TOP and LEFT values to 0 if < 0 , needed for zooming //Corn
	//printf("%d %d %d %d\n", s32(psp_coords_tl.x),s32(psp_coords_tl.y),s32(psp_coords_br.x),s32(psp_coords_br.y));
	sceGuScissor( s32(psp_coords_tl.x) < 0 ? 0 : s32(psp_coords_tl.x), s32(psp_coords_tl.y) < 0 ? 0 : s32(psp_coords_tl.y),
				  s32(psp_coords_br.x), s32(psp_coords_br.y) );
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::Draw2DTexture( f32 frameX, f32 frameY, f32 frameW ,f32 frameH, f32 imageX, f32 imageY, f32 imageW, f32 imageH) 
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

	p_verts[0].pos.x = frameX * mN64ToPSPScale.x + mN64ToPSPTranslate.x; // Frame X Offset * X Scale Factor + Screen X Offset
	p_verts[0].pos.y = frameY * mN64ToPSPScale.y + mN64ToPSPTranslate.y; // Frame Y Offset * Y Scale Factor + Screen Y Offset
	p_verts[0].pos.z = 0.0f;

	p_verts[0].t0.x  = imageX;											 // X coordinates
	p_verts[0].t0.y  = imageY;											 // Y coordinates

	p_verts[1].pos.x = frameW * mN64ToPSPScale.x + mN64ToPSPTranslate.x; // Translated X Offset + (Image Width  * X Scale Factor)
	p_verts[1].pos.y = frameH * mN64ToPSPScale.y + mN64ToPSPTranslate.y; // Translated Y Offset + (Image Height * Y Scale Factor)
	p_verts[1].pos.z = 0.0f;	

	p_verts[1].t0.x  = imageW;											 // X dimentions
	p_verts[1].t0.y  = imageH;											 // Y dimentions

	sceGuDrawArray( GU_SPRITES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, p_verts);
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::Draw2DTextureR( f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3, f32 s, f32 t)	// With Rotation
{
	DAEDALUS_PROFILE( "PSPRenderer::Draw2DTextureR" );
	TextureVtx *p_verts = (TextureVtx*)sceGuGetMemory(4*sizeof(TextureVtx));

	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthMask( GL_TRUE );
	sceGuShadeModel( GU_FLAT );

	sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);

	sceGuEnable(GU_BLEND);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);

	p_verts[0].pos.x = x0 * mN64ToPSPScale.x + mN64ToPSPTranslate.x; 
	p_verts[0].pos.y = y0 * mN64ToPSPScale.y + mN64ToPSPTranslate.y;
	p_verts[0].pos.z = 0.0f;
	p_verts[0].t0.x  = 0.0f;		
	p_verts[0].t0.y  = 0.0f;	

	p_verts[1].pos.x = x1 * mN64ToPSPScale.x + mN64ToPSPTranslate.x; 
	p_verts[1].pos.y = y1 * mN64ToPSPScale.y + mN64ToPSPTranslate.y;
	p_verts[1].pos.z = 0.0f;
	p_verts[1].t0.x  = s;		
	p_verts[1].t0.y  = 0.0f;						

	p_verts[2].pos.x = x2 * mN64ToPSPScale.x + mN64ToPSPTranslate.x;
	p_verts[2].pos.y = y2 * mN64ToPSPScale.y + mN64ToPSPTranslate.y; 
	p_verts[2].pos.z = 0.0f;
	p_verts[2].t0.x  = s;		
	p_verts[2].t0.y  = t;	

	p_verts[3].pos.x = x3 * mN64ToPSPScale.x + mN64ToPSPTranslate.x; 
	p_verts[3].pos.y = y3 * mN64ToPSPScale.y + mN64ToPSPTranslate.y; 
	p_verts[3].pos.z = 0.0f;
	p_verts[3].t0.x  = 0.0f;		
	p_verts[3].t0.y  = t;	

	sceGuDrawArray( GU_TRIANGLE_FAN, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 4, 0, p_verts );
}


//*****************************************************************************
//
//	The following blitting code was taken from The TriEngine.
//	See http://www.assembla.com/code/openTRI for more information.
//
//*****************************************************************************
void PSPRenderer::Draw2DTextureBlit( f32 x, f32 y, f32 width ,f32 height, f32 u0, f32 v0, f32 u1, f32 v1, CNativeTexture * texture) 
{
	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthMask( GL_TRUE );
	sceGuShadeModel( GU_FLAT );

	sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);

	sceGuEnable(GU_BLEND);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);

// 0 Simpler blit algorithm, but doesn't handle big textures as good? (see StarSoldier)
// 1 More complex algorithm. used in newer versions of TriEngine, fixes the main screen in StarSoldier
// Note : We ignore handling height > 512 textures for now
#if 1
	u8* pData = (u8*)( texture->GetData()) ;
	if ( u1 > 512.f )
	{
		s32 off = (u1>u0) ? ((int)u0 & ~31) : ((int)u1 & ~31);

		pData += off * GetBitsPerPixel( texture->GetFormat() );
		u1 -= off;
		u0 -= off;
		sceGuTexImage( 0, Min<u32>(512,texture->GetCorrectedWidth()), Min<u32>(512,texture->GetCorrectedHeight()), texture->GetBlockWidth(), pData );
	}

	f32 start, end;
	f32 cur_u = u0;
	f32 cur_x = x;
	f32 x_end = width;
	f32 slice = 64.f;
	f32 ustep = (u1-u0)/width * slice;

	// blit maximizing the use of the texture-cache
	for( start=0, end=width; start<end; start+=slice )
	{
		TextureVtx *p_verts = (TextureVtx*)sceGuGetMemory(2*sizeof(TextureVtx));

		f32 poly_width = ((cur_x+slice) > x_end) ? (x_end-cur_x) : slice;
		f32 source_width = ((cur_u+ustep) > u1) ? (u1-cur_u) : ustep;

		p_verts[0].t0.x = cur_u;
		p_verts[0].t0.y = v0;
		p_verts[0].pos.x = cur_x * mN64ToPSPScale.x + mN64ToPSPTranslate.x; 
		p_verts[0].pos.y = y	 * mN64ToPSPScale.y + mN64ToPSPTranslate.y; 
		p_verts[0].pos.z = 0;

		cur_u += source_width;
		cur_x += poly_width;

		p_verts[1].t0.x = cur_u;
		p_verts[1].t0.y = v1;
		p_verts[1].pos.x = cur_x * mN64ToPSPScale.x + mN64ToPSPTranslate.x;
		p_verts[1].pos.y = height* mN64ToPSPScale.y + mN64ToPSPTranslate.y;
		p_verts[1].pos.z = 0;

		sceGuDrawArray( GU_SPRITES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, p_verts );
	}
#else
	f32 cur_v = v0;
	f32 cur_y = y;
	f32 v_end = v1;
	f32 y_end = height;
	f32 vslice = 512.f;
	f32 ystep = (height/(v1-v0) * vslice);
	f32 vstep = ((v1-v0) > 0 ? vslice : -vslice);
	
	f32 x_end = width;
	f32 uslice = 64.f;
	//f32 ustep = (u1-u0)/width * xslice;
	f32 xstep = (width/(u1-u0) * uslice);
	f32 ustep = ((u1-u0) > 0 ? uslice : -uslice);

	u8* data = (u8*)(texture->GetData());

	for ( ; cur_y < y_end; cur_y+=ystep, cur_v+=vstep )
	{
		f32 cur_u = u0;
		f32 cur_x = x;
		f32 u_end = u1;

		f32 poly_height = ((cur_y+ystep) > y_end) ? (y_end-cur_y) : ystep;
		f32 source_height = vstep;

		// support negative vsteps
		if ((vstep > 0) && (cur_v+vstep > v_end))
		{
			source_height = (v_end-cur_v);
		}
		else if ((vstep < 0) && (cur_v+vstep < v_end))
		{
			source_height = (cur_v-v_end);
		}
		
		u8* udata = data;
		// blit maximizing the use of the texture-cache
		for( ; cur_x < x_end; cur_x+=xstep, cur_u+=ustep )
		{
			// support large images (width > 512)
			if (cur_u>512.f || cur_u+ustep>512.f)
			{
				s32 off = (ustep>0) ? ((int)cur_u & ~31) : ((int)(cur_u+ustep) & ~31);
	
				udata += off * GetBitsPerPixel( texture->GetFormat() );
				cur_u -= off;
				u_end -= off;
				sceGuTexImage(0, Min<u32>(512,texture->GetCorrectedWidth()), Min<u32>(512,texture->GetCorrectedHeight()), texture->GetBlockWidth(), udata);
			}
			TextureVtx *p_verts = (TextureVtx*)sceGuGetMemory(2*sizeof(TextureVtx));

			//f32 poly_width = ((cur_x+xstep) > x_end) ? (x_end-cur_x) : xstep;
			f32 poly_width = xstep;
			f32 source_width = ustep;

			// support negative usteps
			if ((ustep > 0) && (cur_u+ustep > u_end))
			{
				source_width = (u_end-cur_u);
			}
			else if ((ustep < 0) && (cur_u+ustep < u_end))
			{
				source_width = (cur_u-u_end);
			}

			p_verts[0].t0.x = cur_u;
			p_verts[0].t0.y = cur_v;
			p_verts[0].pos.x = cur_x; 
			p_verts[0].pos.y = cur_y; 
			p_verts[0].pos.z = 0;

			p_verts[1].t0.x = cur_u + source_width;
			p_verts[1].t0.y = cur_v + source_height;
			p_verts[1].pos.x = cur_x + poly_width;
			p_verts[1].pos.y = cur_y + poly_height;
			p_verts[1].pos.z = 0;

			sceGuDrawArray( GU_SPRITES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, p_verts );
		}
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::SetProjection(const u32 address, bool bPush, bool bReplace)
{
	// Projection
	if (bPush)
	{
		if (mProjectionTop >= (MATRIX_STACK_SIZE-2))
			DBGConsole_Msg(0, "Pushing past proj stack limits! %d/%d", mProjectionTop, MATRIX_STACK_SIZE);
		else
			++mProjectionTop;

		if (bReplace)
		{
			// Load projection matrix
			MatrixFromN64FixedPoint( mProjectionStack[mProjectionTop], address);
		}
		else
		{
			MatrixFromN64FixedPoint( mProjectionStack[mProjectionTop], address);
		#ifdef DAEDALUS_PSP_USE_VFPU
			matrixMultiplyAligned( &mProjectionStack[mProjectionTop], &mProjectionStack[mProjectionTop], &mProjectionStack[mProjectionTop-1] );
		#else	
			mProjectionStack[mProjectionTop] = mProjectionStack[mProjectionTop] * mProjectionStack[mProjectionTop-1];
		#endif
		}
	}
	else
	{
		if (bReplace)
		{
			// Load projection matrix
			MatrixFromN64FixedPoint( mProjectionStack[mProjectionTop], address);

			//Hack needed to show heart in OOT & MM
			//it renders at Z cordinate = 0.0f that gets clipped away.
			//so we translate them a bit along Z to make them stick :) //Corn
			//
			if( g_ROM.ZELDA_HACK ) mProjectionStack[mProjectionTop].mRaw[14] += 0.4f;
		}
		else
		{
			MatrixFromN64FixedPoint( mProjectionStack[mProjectionTop+1], address);
		#ifdef DAEDALUS_PSP_USE_VFPU
			matrixMultiplyAligned( &mProjectionStack[mProjectionTop], &mProjectionStack[mProjectionTop+1], &mProjectionStack[mProjectionTop] );
		#else	
			mProjectionStack[mProjectionTop] = mProjectionStack[mProjectionTop+1] * mProjectionStack[mProjectionTop];
		#endif
		}
	}

	mWorldProjectValid = false;
	sceGuSetMatrix( GU_PROJECTION, reinterpret_cast< const ScePspFMatrix4 * >( &mProjectionStack[mProjectionTop]) );

	DL_PF("    Level = %d\n"
		"    %#+12.5f %#+12.5f %#+12.7f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.7f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.7f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.7f %#+12.5f\n",
		mProjectionTop,
		mProjectionStack[mProjectionTop].m[0][0], mProjectionStack[mProjectionTop].m[0][1], mProjectionStack[mProjectionTop].m[0][2], mProjectionStack[mProjectionTop].m[0][3],
		mProjectionStack[mProjectionTop].m[1][0], mProjectionStack[mProjectionTop].m[1][1], mProjectionStack[mProjectionTop].m[1][2], mProjectionStack[mProjectionTop].m[1][3],
		mProjectionStack[mProjectionTop].m[2][0], mProjectionStack[mProjectionTop].m[2][1], mProjectionStack[mProjectionTop].m[2][2], mProjectionStack[mProjectionTop].m[2][3],
		mProjectionStack[mProjectionTop].m[3][0], mProjectionStack[mProjectionTop].m[3][1], mProjectionStack[mProjectionTop].m[3][2], mProjectionStack[mProjectionTop].m[3][3]);
}

//*****************************************************************************
//
//*****************************************************************************
void PSPRenderer::SetWorldView(const u32 address, bool bPush, bool bReplace)
{
	// ModelView
	if (bPush)
	{
		if (mModelViewTop >= (MATRIX_STACK_SIZE-2))
			DBGConsole_Msg(0, "Pushing past modelview stack limits! %d/%d", mModelViewTop, MATRIX_STACK_SIZE);
		else
			++mModelViewTop;

		// We should store the current projection matrix...
		if (bReplace)
		{
			// Load ModelView matrix
			MatrixFromN64FixedPoint( mModelViewStack[mModelViewTop], address);
			//Hack to make GEX games work, need to multiply all elements with 2.0 //Corn
			if( g_ROM.GameHacks == GEX_GECKO ) for(u32 i=0;i<16;i++) mModelViewStack[mModelViewTop].mRaw[i] += mModelViewStack[mModelViewTop].mRaw[i];
		}
		else	// Multiply ModelView matrix
		{
			MatrixFromN64FixedPoint( mModelViewStack[mModelViewTop], address);
		#ifdef DAEDALUS_PSP_USE_VFPU
			matrixMultiplyAligned( &mModelViewStack[mModelViewTop], &mModelViewStack[mModelViewTop], &mModelViewStack[mModelViewTop-1] );
		#else	
			mModelViewStack[mModelViewTop] = mModelViewStack[mModelViewTop] * mModelViewStack[mModelViewTop-1];
		#endif
		}
	}
	else	// NoPush
	{
		if (bReplace)
		{
			// Load ModelView matrix
			MatrixFromN64FixedPoint( mModelViewStack[mModelViewTop], address);
		}
		else
		{
			// Multiply ModelView matrix
			MatrixFromN64FixedPoint( mModelViewStack[mModelViewTop+1], address);
		#ifdef DAEDALUS_PSP_USE_VFPU
			matrixMultiplyAligned( &mModelViewStack[mModelViewTop], &mModelViewStack[mModelViewTop+1], &mModelViewStack[mModelViewTop] );
		#else	
			mModelViewStack[mModelViewTop] = mModelViewStack[mModelViewTop+1] * mModelViewStack[mModelViewTop];
		#endif
		}
	}

	mWorldProjectValid = false;

	DL_PF("    Level = %d\n"
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n",
		mModelViewTop,
		mModelViewStack[mModelViewTop].m[0][0], mModelViewStack[mModelViewTop].m[0][1], mModelViewStack[mModelViewTop].m[0][2], mModelViewStack[mModelViewTop].m[0][3],
		mModelViewStack[mModelViewTop].m[1][0], mModelViewStack[mModelViewTop].m[1][1], mModelViewStack[mModelViewTop].m[1][2], mModelViewStack[mModelViewTop].m[1][3],
		mModelViewStack[mModelViewTop].m[2][0], mModelViewStack[mModelViewTop].m[2][1], mModelViewStack[mModelViewTop].m[2][2], mModelViewStack[mModelViewTop].m[2][3],
		mModelViewStack[mModelViewTop].m[3][0], mModelViewStack[mModelViewTop].m[3][1], mModelViewStack[mModelViewTop].m[3][2], mModelViewStack[mModelViewTop].m[3][3]);
}

//*****************************************************************************
//
//*****************************************************************************
inline Matrix4x4 & PSPRenderer::GetWorldProject()
{
	if( !mWorldProjectValid )
	{
		mWorldProjectValid = true;
		if( mReloadProj )
		{
			mReloadProj = false;
			sceGuSetMatrix( GU_PROJECTION, reinterpret_cast< const ScePspFMatrix4 * >( &mProjectionStack[mProjectionTop]) );
		}
	#ifdef DAEDALUS_PSP_USE_VFPU
		matrixMultiplyAligned( &mWorldProject, &mModelViewStack[mModelViewTop], &mProjectionStack[mProjectionTop] );
	#else	
		mWorldProject = mModelViewStack[mModelViewTop] * mProjectionStack[mProjectionTop];
	#endif
	}

	return mWorldProject;
}

//*****************************************************************************
//
//*****************************************************************************
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
void PSPRenderer::PrintActive()
{
	const Matrix4x4 & mat( GetWorldProject() );
	DL_PF(
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n",
		mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
		mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
		mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
		mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
}
#endif

//*************************************************************************************
// 
//*************************************************************************************
void PSPRenderer::MatrixFromN64FixedPoint( Matrix4x4 & mat, u32 address )
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (address + 64 > MAX_RAM_ADDRESS)
	{
		DBGConsole_Msg(0, "Mtx: Address invalid (0x%08x)", address);
		return;
	}
#endif

#if 1 //1->looped, 0->unrolled //Corn
	const f32 fRecip = 1.0f / 65536.0f;
	const u8 *ptr( g_pu8RamBase + address );
	s16 hi;
	u16 lo;


	for (u32 i = 0; i < 4; i++)
	{
		hi = *(s16 *)(ptr + (i<<3) + ((0     )^0x2));
		lo = *(u16 *)(ptr + (i<<3) + ((0 + 32)^0x2));
		mat.m[i][0] = ((hi<<16) | (lo)) * fRecip;

		hi = *(s16 *)(ptr + (i<<3) + ((2     )^0x2));
		lo = *(u16 *)(ptr + (i<<3) + ((2 + 32)^0x2));
		mat.m[i][1] = ((hi<<16) | (lo)) * fRecip;

		hi = *(s16 *)(ptr + (i<<3) + ((4     )^0x2));
		lo = *(u16 *)(ptr + (i<<3) + ((4 + 32)^0x2));
		mat.m[i][2] = ((hi<<16) | (lo)) * fRecip;

		hi = *(s16 *)(ptr + (i<<3) + ((6     )^0x2));
		lo = *(u16 *)(ptr + (i<<3) + ((6 + 32)^0x2));
		mat.m[i][3] = ((hi<<16) | (lo)) * fRecip;
	}

#else
	struct N64Imat
	{
		s16	H01, H00, H03, H02;
		s16	H11, H10, H13, H12;
		s16	H21, H20, H23, H22;
		s16	H31, H30, H33, H32;

		u16	L01, L00, L03, L02;
		u16	L11, L10, L13, L12;
		u16	L21, L20, L23, L22;
		u16	L31, L30, L33, L32;
	};

	const N64Imat *Imat = (N64Imat *)( g_pu8RamBase + address );
	const f32 fRecip = 1.0f / 65536.0f;

	mat.m[0][0] = (f32)((Imat->H00 << 16) | (Imat->L00)) * fRecip;
	mat.m[0][1] = (f32)((Imat->H01 << 16) | (Imat->L01)) * fRecip;
	mat.m[0][2] = (f32)((Imat->H02 << 16) | (Imat->L02)) * fRecip;
	mat.m[0][3] = (f32)((Imat->H03 << 16) | (Imat->L03)) * fRecip;

	mat.m[1][0] = (f32)((Imat->H10 << 16) | (Imat->L10)) * fRecip;
	mat.m[1][1] = (f32)((Imat->H11 << 16) | (Imat->L11)) * fRecip;
	mat.m[1][2] = (f32)((Imat->H12 << 16) | (Imat->L12)) * fRecip;
	mat.m[1][3] = (f32)((Imat->H13 << 16) | (Imat->L13)) * fRecip;

	mat.m[2][0] = (f32)((Imat->H20 << 16) | (Imat->L20)) * fRecip;
	mat.m[2][1] = (f32)((Imat->H21 << 16) | (Imat->L21)) * fRecip;
	mat.m[2][2] = (f32)((Imat->H22 << 16) | (Imat->L22)) * fRecip;
	mat.m[2][3] = (f32)((Imat->H23 << 16) | (Imat->L23)) * fRecip;

	mat.m[3][0] = (f32)((Imat->H30 << 16) | (Imat->L30)) * fRecip;
	mat.m[3][1] = (f32)((Imat->H31 << 16) | (Imat->L31)) * fRecip;
	mat.m[3][2] = (f32)((Imat->H32 << 16) | (Imat->L32)) * fRecip;
	mat.m[3][3] = (f32)((Imat->H33 << 16) | (Imat->L33)) * fRecip;
#endif
}
//*****************************************************************************
//Modify the WorldProject matrix, used by Kirby & SSB //Corn
//*****************************************************************************
void PSPRenderer::InsertMatrix(u32 w0, u32 w1)
{
	mWPmodified = true;	//Signal that Worldproject matrix is changed

	//Make sure WP matrix is up to date before changing WP matrix
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
		mWorldProject.m[y][x]   = (f32)(s32)mWorldProject.m[y][x] + ((f32)(w1 >> 16) / 65536.0f);
		mWorldProject.m[y][x+1] = (f32)(s32)mWorldProject.m[y][x+1] + ((f32)(w1 & 0xFFFF) / 65536.0f);
	}
	else
	{
		//Change integer part
		mWorldProject.m[y][x]	= (f32)(s16)(w1 >> 16);
		mWorldProject.m[y][x+1] = (f32)(s16)(w1 & 0xFFFF);
	}

	DL_PF(
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n",
		mWorldProject.m[0][0], mWorldProject.m[0][1], mWorldProject.m[0][2], mWorldProject.m[0][3],
		mWorldProject.m[1][0], mWorldProject.m[1][1], mWorldProject.m[1][2], mWorldProject.m[1][3],
		mWorldProject.m[2][0], mWorldProject.m[2][1], mWorldProject.m[2][2], mWorldProject.m[2][3],
		mWorldProject.m[3][0], mWorldProject.m[3][1], mWorldProject.m[3][2], mWorldProject.m[3][3]);
}

//*****************************************************************************
//Replaces the WorldProject matrix //Corn
//*****************************************************************************
void PSPRenderer::ForceMatrix(const u32 address)
{
	mWorldProjectValid = true;
	mWPmodified = true;	//Signal that Worldproject matrix is changed

	MatrixFromN64FixedPoint( mWorldProject, address );

	DL_PF(
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		"    %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n",
		mWorldProject.m[0][0], mWorldProject.m[0][1], mWorldProject.m[0][2], mWorldProject.m[0][3],
		mWorldProject.m[1][0], mWorldProject.m[1][1], mWorldProject.m[1][2], mWorldProject.m[1][3],
		mWorldProject.m[2][0], mWorldProject.m[2][1], mWorldProject.m[2][2], mWorldProject.m[2][3],
		mWorldProject.m[3][0], mWorldProject.m[3][1], mWorldProject.m[3][2], mWorldProject.m[3][3]);
}